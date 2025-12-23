import struct
from enum import Enum

# Protocol constants
SERVER_VERSION = 2   
HEADER_SIZE = 7       # Header size without clientID (includes: version, code, payloadSize)
CLIENT_ID_SIZE = 16
MSG_ID_SIZE = 4
MSG_TYPE_MAX = 0xFF
MSG_ID_MAX = 0xFFFFFFFF
NAME_SIZE = 255
PUBLIC_KEY_SIZE = 160

# Request Codes
class REQCode(Enum):
    REQUEST_REGISTRATION = 600   # Registration request (UUID ignored)
    REQUEST_USERS_LIST = 601       # Users list request (payload invalid; payloadSize = 0)
    REQUEST_PULL_USER_PUBLIC_KEY = 602
    REQUEST_SEND_MSG_TO_USER = 603
    REQUEST_PULL_PENDING_MSGS = 604   # Pending messages request (payload invalid; payloadSize = 0)

# Response Codes
class RSPCode(Enum):
    RESPONSE_REGISTRATION_SUCSSES = 2100
    RESPONSE_USERS_LIST = 2101
    RESPONSE_PUBLIC_KEY = 2102
    RESPONSE_MSG_SENT_TO_SERVER = 2103
    RESPONSE_PULL_PENDING_MSGS = 2104
    RESPONSE_GENERAL_ERROR = 9000       # General error response (payload invalid; payloadSize = 0)

class REQHeader:
    def __init__(self):
        self.version = 0      
        self.code = 0         
        self.clientID = b""
        self.payloadSize = 0
        self.SIZE = CLIENT_ID_SIZE + HEADER_SIZE

    def from_bytes(self, data):
        """Little Endian: convert the Request Header from binary data."""
        try:
            # Extract clientID from the first CLIENT_ID_SIZE bytes.
            self.clientID = struct.unpack(f"<{CLIENT_ID_SIZE}s", data[:CLIENT_ID_SIZE])[0]
            headerData = data[CLIENT_ID_SIZE:CLIENT_ID_SIZE + HEADER_SIZE]
            # Unpack version (B), code (H), and payloadSize (L)
            self.version, self.code, self.payloadSize = struct.unpack("<BHL", headerData)
            return True
        except:
            self.__init__()  
            return False

class REQRegistration:
    def __init__(self):
        self.header = REQHeader()
        self.name = b""
        self.publicKey = b""

    def from_bytes(self, data):
        """Little Endian: convert Request Header and Registration data from binary data."""
        if not self.header.from_bytes(data):
            return False
        try:
            # Extract name data (trim after the null terminating character)
            nameData = data[self.header.SIZE:self.header.SIZE + NAME_SIZE]
            self.name = str(struct.unpack(f"<{NAME_SIZE}s", nameData)[0].partition(b'\0')[0].decode('utf-8'))
            # Extract public key data
            keyData = data[self.header.SIZE + NAME_SIZE:self.header.SIZE + NAME_SIZE + PUBLIC_KEY_SIZE]
            self.publicKey = struct.unpack(f"<{PUBLIC_KEY_SIZE}s", keyData)[0]
            return True
        except:
            self.name = b""
            self.publicKey = b""
            return False

class REQPublicKey:
    def __init__(self):
        self.header = REQHeader()
        self.clientID = b""

    def from_bytes(self, data):
        """Little Endian: convert Request Header and client ID from binary data."""
        if not self.header.from_bytes(data):
            return False
        try:
            clientID = data[self.header.SIZE:self.header.SIZE + CLIENT_ID_SIZE]
            self.clientID = struct.unpack(f"<{CLIENT_ID_SIZE}s", clientID)[0]
            return True
        except:
            self.clientID = b""
            return False

class REQSendMessage:
    def __init__(self):
        self.header = REQHeader()
        self.clientID = b""
        self.messageType = 0
        self.contentSize = 0
        self.content = b""

    def from_bytes(self, conn, data):
        """
        Little Endian: convert Request Header and message data from binary data.
        Reads additional chunks from the socket if necessary until all content is obtained.
        """
        remainingData = data
        if not self.header.from_bytes(remainingData):
            return False

        try:
            offset = self.header.SIZE
            # Unpack clientID (16 bytes)
            self.clientID = struct.unpack(f"<{CLIENT_ID_SIZE}s", remainingData[offset:offset + CLIENT_ID_SIZE])[0]
            offset += CLIENT_ID_SIZE

            # Unpack messageType (1 byte) and contentSize (4 bytes)
            self.messageType, self.contentSize = struct.unpack("<BL", remainingData[offset:offset + 5])
            offset += 5

            # Calculate how many bytes were read for the content
            bytesReadFromData = len(remainingData) - offset
            if bytesReadFromData > self.contentSize:
                bytesReadFromData = self.contentSize

            # Read initial content bytes
            self.content = struct.unpack(f"<{bytesReadFromData}s", remainingData[offset:offset + bytesReadFromData])[0]
            totalBytesRead = bytesReadFromData

            # Continue reading from the socket if content is incomplete
            while totalBytesRead < self.contentSize:
                chunkSize = self.contentSize - totalBytesRead
                chunk = conn.recv(chunkSize)
                if not chunk:
                    raise ValueError("Socket closed prematurely while reading message content.")
                self.content += struct.unpack(f"<{len(chunk)}s", chunk)[0]
                totalBytesRead += len(chunk)

            return True
        except:
            self.clientID = b""
            self.messageType = 0
            self.contentSize = 0
            self.content = b""
            return False

class RESHeader:
    def __init__(self, code):
        self.version = SERVER_VERSION  
        self.code = code               
        self.payloadSize = 0     
        self.SIZE = HEADER_SIZE

    def to_bytes(self):
        """Little Endian: convert the Response Header into binary data."""
        try:
            return struct.pack("<BHL", self.version, self.code, self.payloadSize)
        except:
            return b""

class RESRegistration:
    def __init__(self):
        self.header = RESHeader(RSPCode.RESPONSE_REGISTRATION_SUCSSES.value)
        self.clientID = b""

    def to_bytes(self):
        """Little Endian: convert the Response Header and client ID into binary data."""
        try:
            data = self.header.to_bytes()
            data += struct.pack(f"<{CLIENT_ID_SIZE}s", self.clientID)
            return data
        except:
            return b""

class RESPublicKey:
    def __init__(self):
        self.header = RESHeader(RSPCode.RESPONSE_PUBLIC_KEY.value)
        self.clientID = b""
        self.publicKey = b""

    def to_bytes(self):
        """Little Endian: convert the Response Header and Public Key into binary data."""
        try:
            data = self.header.to_bytes()
            data += struct.pack(f"<{CLIENT_ID_SIZE}s", self.clientID)
            data += struct.pack(f"<{PUBLIC_KEY_SIZE}s", self.publicKey)
            return data
        except:
            return b""

class RESMessageSend:
    def __init__(self):
        self.header = RESHeader(RSPCode.RESPONSE_MSG_SENT_TO_SERVER.value)
        self.clientID = b""
        self.messageID = b""

    def to_bytes(self):
        """Little Endian: convert the Response Header and client ID with message ID into binary data."""
        try:
            data = self.header.to_bytes()
            data += struct.pack(f"<{CLIENT_ID_SIZE}sL", self.clientID, self.messageID)
            return data
        except:
            return b""

class PendingMessage:
    def __init__(self):
        self.messageClientID = b""
        self.messageID = 0
        self.messageType = 0
        self.messageSize = 0
        self.content = b""

    def to_bytes(self):
        """Little Endian: convert the Pending Message into binary data."""
        try:
            data = struct.pack(f"<{CLIENT_ID_SIZE}s", self.messageClientID)
            data += struct.pack("<LBL", self.messageID, self.messageType, self.messageSize)
            data += struct.pack(f"<{self.messageSize}s", self.content)
            return data
        except:
            return b""
