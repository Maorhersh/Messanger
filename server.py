from datetime import datetime
import re
import logging
import socket
import uuid
import database
import selectors
import protocol

# Dictionary for storing request handler functions
request_handlers = {}

def register_handler(code):
    """Decorator that registers a function as a handler for a specific request code."""
    def decorator(func):
        request_handlers[code] = func
        return func
    return decorator

class Server:
    DATABASE = 'defensive.db'
    PACKET_SIZE = 1024       
    MAX_USERS = 5           
    BLOCK_USAGE = False      

    def __init__(self, port):
        """Constructor of the server - initializes message formats and other requirements."""
        self.logger = logging.getLogger("Server")
        handler = logging.StreamHandler()
        formatter = logging.Formatter('[%(asctime)s]: %(message)s', '%d/%m/%Y  %H:%M:%S')
        handler.setFormatter(formatter)
        if not self.logger.handlers:
            self.logger.addHandler(handler)
        self.logger.setLevel(logging.INFO)
        self.sel = selectors.DefaultSelector()
        self.requestHandle = request_handlers
        self.database = database.Database(Server.DATABASE)
        self.port = port

    def pack_pending(msg):
        """
        this is a helper function to pack a pending message from a database record.
        """
        pending = protocol.PendingMessage()
        pending.messageID = int(msg[0])
        pending.messageClientID = msg[1]
        pending.messageType = int(msg[2])
        pending.content = msg[3]
        pending.messageSize = len(msg[3])
        return pending.to_bytes()

    def accept_connection(self, sock, mask):
        """Accept a new connection and register it for reading."""
        conn, address = sock.accept()
        conn.setblocking(Server.BLOCK_USAGE)
        self.sel.register(conn, selectors.EVENT_READ, self.read_requests)

    def read_requests(self, conn, mask):
        """Read data from a client, parse the request, and pass it to the proper handler."""
        self.logger.info("A client has connected.")
        data = conn.recv(Server.PACKET_SIZE)
        if not data:
            self.sel.unregister(conn)
            conn.close()
            return

        reqHeader = protocol.REQHeader()
        if not reqHeader.from_bytes(data):
            self.logger.error("Failed to parse request header!")
            resHeader = protocol.RESHeader(protocol.RSPCode.RESPONSE_GENERAL_ERROR.value)
            self.send_response(conn, resHeader.to_bytes())
        elif reqHeader.code not in self.requestHandle:
            self.logger.error(f"Unhandled request code: {reqHeader.code}")
            resHeader = protocol.RESHeader(protocol.RSPCode.RESPONSE_GENERAL_ERROR.value)
            self.send_response(conn, resHeader.to_bytes())
        else:
            if not self.requestHandle[reqHeader.code](self, conn, data):
                resHeader = protocol.RESHeader(protocol.RSPCode.RESPONSE_GENERAL_ERROR.value)
                self.send_response(conn, resHeader.to_bytes())

        self.sel.unregister(conn)
        conn.close()

    def send_response(self, conn, data):
        """Send a response back to the client in chunks."""
        size = len(data)
        for offset in range(0, size, Server.PACKET_SIZE):
            chunk = data[offset:offset + Server.PACKET_SIZE]
            if len(chunk) < Server.PACKET_SIZE:
                chunk += bytearray(Server.PACKET_SIZE - len(chunk))
            try:
                sent = conn.send(chunk)
                if sent != len(chunk):
                    self.logger.error(f"Failed to send entire chunk. Sent {sent}/{len(chunk)} bytes.")
                    return False
            except Exception as e:
                self.logger.error(f"Failed to send response: {e}")
                return False
        self.logger.info("Response sent successfully.")
        return True

    def run_server(self):
        """Start the server and run the main event loop."""
        self.database.initialize()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.bind(("", self.port))
            sock.listen(Server.MAX_USERS)
            sock.setblocking(Server.BLOCK_USAGE)
            self.sel.register(sock, selectors.EVENT_READ, self.accept_connection)
        except socket.error as se:
            self.logger.error(f"Socket Error: {se}")
            return False
        except Exception as e:
            self.logger.error(f"Unexpected Error: {e}")
            return False

        print(f"The Server is connected on port {self.port}..")
        while True:
            try:
                events = self.sel.select()
                for key, mask in events:
                    callback = key.data
                    callback(key.fileobj, mask)
            except socket.error as se:
                self.logger.error(f"Socket Error  {se}")
            except Exception as e:
                self.logger.exception(f"Server exception: {e}")

    @register_handler(protocol.REQCode.REQUEST_REGISTRATION.value)
    def handle_REQRegistration(self, conn, data):
        """Register a new client and store it in the database."""
        request = protocol.REQRegistration()
        response = protocol.RESRegistration()
        if not request.from_bytes(data):
            self.logger.error("Server has failed to parse registration request.")
            return False

        if not re.match(r'^[a-zA-Z0-9_]+$', request.name):
            self.logger.info(f"Registration Request: Username is not written according to standards ({request.name})")
            return False

        try:
            if self.database.client_username_exists(request.name):
                self.logger.info(f"Server has detected that the username already exists ({request.name}).")
                return False
        except Exception as e:
            self.logger.error(f"Database error - {e}")
            return False

        client = database.Client(uuid.uuid4().hex, request.name, request.publicKey)
        if not self.database.store_client(client):
            self.logger.error(f"Server has failed to store client {request.name}.")
            return False

        self.logger.info(f"Server registered client successfully {request.name}.")
        response.clientID = client.ID
        response.header.payloadSize = protocol.CLIENT_ID_SIZE
        return self.send_response(conn, response.to_bytes())

    @register_handler(protocol.REQCode.REQUEST_USERS_LIST.value)
    def handle_users_list_Request(self, conn, data):
        """Send a list of clients in response to a request."""
        request = protocol.REQHeader()
        if not request.from_bytes(data):
            self.logger.error("Server has failed to parse request header for users list request!")
            return False
        try:
            if not self.database.client_id_exists(request.clientID):
                self.logger.info(f"Server has detected that client ID does not exist for users list request ({request.clientID})!")
                return False
        except Exception as e:
            self.logger.error(f"Database error - {e}")
            return False
        response = protocol.RESHeader(protocol.RSPCode.RESPONSE_USERS_LIST.value)
        clients = self.database.get_clients_list()
        payload_parts = []
        for user in clients:
            client_id, name = user  # Assume 'user' is a tuple (clientID, name)
            if client_id != request.clientID:
                # Append the clientID to the payload parts.
                payload_parts.append(client_id)
                # Calculate the number of bytes required to pad the name.
                padding_length = protocol.NAME_SIZE - len(name)
                # Convert to UTF-8 if necessary and pad with null bytes to complete the name size.
                name_bytes = name + (b'\0' * padding_length)
                payload_parts.append(name_bytes)

        payload = b"".join(payload_parts)
        response.payloadSize = len(payload)
        self.logger.info(f"Users list has been created successfully for clientID ({request.clientID}).")
        return self.send_response(conn, response.to_bytes() + payload)

    @register_handler(protocol.REQCode.REQUEST_PULL_USER_PUBLIC_KEY.value)
    def handle_REQPublicKey(self, conn, data):
        """Send the public key for a requested client."""
        request = protocol.REQPublicKey()
        response = protocol.RESPublicKey()
        if not request.from_bytes(data):
            self.logger.error("PublicKey Request: Failed to parse request header!")
            return False
        key = self.database.get_client_public_key(request.clientID)
        if not key:
            self.logger.info("PublicKey Request: clientID doesn't exist.")
            return False
        response.publicKey = key
        response.clientID = request.clientID
        response.header.payloadSize = protocol.CLIENT_ID_SIZE + protocol.PUBLIC_KEY_SIZE
        self.logger.info(f"Public Key response was successfully built for clientID ({request.clientID}).")
        return self.send_response(conn, response.to_bytes())

    @register_handler(protocol.REQCode.REQUEST_SEND_MSG_TO_USER.value)
    def handle_REQSendMessage(self, conn, data):
        """Store a message from one client to another."""
        request = protocol.REQSendMessage()
        response = protocol.RESMessageSend()
        if not request.from_bytes(conn, data):
            self.logger.error("Send Message Request: Failed to parse request header!")
            return False

        msg = database.Message(request.clientID,
                               request.header.clientID,
                               request.messageType,
                               request.content)

        msgId = self.database.store_message(msg)
        if not msgId:
            self.logger.error("Send Message Request: Failed to store msg.")
            return False

        response.header.payloadSize = protocol.CLIENT_ID_SIZE + protocol.MSG_ID_SIZE
        response.clientID = request.clientID
        response.messageID = msgId
        self.logger.info(f"Message from clientID ({request.header.clientID}) successfully stored.")
        return self.send_response(conn, response.to_bytes())

    @register_handler(protocol.REQCode.REQUEST_PULL_PENDING_MSGS.value)
    def handle_pending_messages_request(self, conn, data):
        """Send pending messages for a client."""
        request = protocol.REQHeader()
        response = protocol.RESHeader(protocol.RSPCode.RESPONSE_PULL_PENDING_MSGS.value)
        if not request.from_bytes(data):
            self.logger.error("Pending messages request: Failed to parse request header!")
            return False

        try:
            if not self.database.client_id_exists(request.clientID):
                self.logger.info(f"Pending messages request: clientID ({request.clientID}) does not exist!")
                return False
        except Exception as e:
            self.logger.error(f"Pending messages request: Database error - {e}")
            return False

        messages = self.database.get_pending_messages(request.clientID)
        ids = [int(msg[0]) for msg in messages]
        
        payload = b"".join(Server.pack_pending(msg) for msg in messages)
        response.payloadSize = len(payload)
        self.logger.info(f"Pending messages for clientID ({request.clientID}) successfully extracted.")
        
        if self.send_response(conn, response.to_bytes() + payload):
            for msg_id in ids:
                self.database.remove_message(msg_id)
            return True
        return False

if __name__ == '__main__':
    PORT_INFO_FILE = "myport.info"
    default_port = 1357 
    try:
        with open(PORT_INFO_FILE, "r") as f:
            port = int(f.readline().strip())
    except (FileNotFoundError, ValueError):
        print("Error while opening port info file, running default port")
        port = default_port
    server = Server(port) 
    if not server.run_server():
       print("Error in running the server")
       exit()
