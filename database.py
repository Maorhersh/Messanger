import logging
import sqlite3
import protocol

class Client:
    def __init__(self, client_id, client_name, public_key):
        self.ID = bytes.fromhex(client_id)  # Unique client ID, 16 bytes.
        self.Name = client_name             # Client's name, 255 bytes.
        self.PublicKey = public_key         # Client's public key, 160 bytes.

    def check_correctness(self):
        if not self.ID or len(self.ID) != protocol.CLIENT_ID_SIZE:
            return False
        if not self.Name or len(self.Name.encode('utf-8')) >= protocol.NAME_SIZE:
            return False
        if not self.PublicKey or len(self.PublicKey) != protocol.PUBLIC_KEY_SIZE:
            return False
        return True

class Message:
    def __init__(self, target_ID, source_ID, mtype, content):
        self.ID = 0                  
        self.source_client = source_ID # Sender's client ID, 16 bytes.
        self.target_client = target_ID     # Receiver's client ID, 16 bytes.
        self.Type = mtype             # Message type, 1 byte.
        self.Content = content        # Message content (Blob).

    def check_correctness(self):
        # Check that the receiver's ID is valid.
        if not self.target_client or len(self.target_client) != protocol.CLIENT_ID_SIZE:
            return False
        # Check that the sender's ID is valid.
        if not self.source_client or len(self.source_client) != protocol.CLIENT_ID_SIZE:
            return False
        # Check that the message type is valid (non-zero and within allowed range).
        if not self.Type or self.Type > protocol.MSG_TYPE_MAX:
            return False
        return True

class Database:
    CLIENTS = 'clients'
    MESSAGES = 'messages'

    def __init__(self, name):
        self.name = name  # Database filename.

    def connect(self):
        conn = sqlite3.connect(self.name)
        conn.text_factory = bytes
        return conn

    def executescript(self, script):
        try:
            with self.connect() as conn:
                conn.executescript(script)
                conn.commit()
        except Exception as e:
            logging.exception(f"Database executescript error: {e}")

    def execute(self, query, args, commit=False, get_last_row=False):
        try:
            with self.connect() as conn:
                cur = conn.cursor()
                cur.execute(query, args)
                if commit:
                    conn.commit()
                    if get_last_row:
                        return cur.lastrowid
                    return True
                return cur.fetchall()
        except sqlite3.IntegrityError as e:
            logging.error(f"Database integrity error: {e}")
            return None
        except sqlite3.Error as e:
            logging.error(f"General database error: {e}")
            return None
        except Exception as e:
            logging.exception(f"Unexpected error in execute: {e}")
            return None

    def initialize(self):
        # SQL script to create the clients table (without LastSeen field).
        create_clients_script = f"""
            CREATE TABLE IF NOT EXISTS {Database.CLIENTS}(
              ID CHAR(16) NOT NULL PRIMARY KEY,
              Name CHAR(255) NOT NULL,
              PublicKey CHAR(160) NOT NULL
            );
            """
        # SQL script to create the messages table and an index on target_client.
        create_messages_script = f"""
            CREATE TABLE IF NOT EXISTS {Database.MESSAGES}(
              ID INTEGER PRIMARY KEY,
              target_client CHAR(16) NOT NULL,
              source_client CHAR(16) NOT NULL,
              Type CHAR(1) NOT NULL,
              Content BLOB,
              FOREIGN KEY(target_client) REFERENCES {Database.CLIENTS}(ID),
              FOREIGN KEY(source_client) REFERENCES {Database.CLIENTS}(ID)
            );
            
            CREATE INDEX IF NOT EXISTS idx_target_ID 
            ON {Database.MESSAGES}(target_client);
            """
        self.executescript(create_clients_script)
        self.executescript(create_messages_script)

    def client_username_exists(self, username):
        # Check if a client with the given username exists.
        results = self.execute(
            f"SELECT 1 FROM {Database.CLIENTS} WHERE Name = :username LIMIT 1",
            {"username": username}
        )
        return bool(results)

    def client_id_exists(self, client_id):
        # Check if a client with the given ID exists.
        results = self.execute(
            f"SELECT 1 FROM {Database.CLIENTS} WHERE ID = :client_id LIMIT 1",
            {"client_id": client_id}
        )
        return bool(results)

    def store_client(self, client):
        # Insert a new client into the database after verifying its correctness.
        if not isinstance(client, Client) or not client.check_correctness():
            return False
        return self.execute(
            f"INSERT INTO {Database.CLIENTS} VALUES (:id, :name, :public_key)",
            {"id": client.ID, "name": client.Name, "public_key": client.PublicKey},
            commit=True
        )

    def store_message(self, msg):
        # Insert a new message into the database after verifying its correctness.
        if not isinstance(msg, Message) or not msg.check_correctness():
            return False
        return self.execute(
            f"INSERT INTO {Database.MESSAGES}(target_client, source_client, Type, Content) VALUES (:target_ID, :source_ID, :type, :content)",
            {"target_ID": msg.target_client,
             "source_ID": msg.source_client,
             "type": msg.Type,
             "content": msg.Content},
            commit=True,
            get_last_row=True
        )

    def remove_message(self, msg_id):
        # Delete a message by its ID.
        return self.execute(
            f"DELETE FROM {Database.MESSAGES} WHERE ID = :msg_id",
            {"msg_id": msg_id},
            commit=True
        )

    def get_clients_list(self):
        # Retrieve a list of all clients (their IDs and Names).
        return self.execute(
            f"SELECT ID, Name FROM {Database.CLIENTS}",
            {}
        )

    def get_client_public_key(self, client_id):
        # Retrieve the public key for a given client ID.
        results = self.execute(
            f"SELECT PublicKey FROM {Database.CLIENTS} WHERE ID = :client_id",
            {"client_id": client_id}
        )
        return results[0][0] if results else None

    def get_pending_messages(self, client_id):
        # Retrieve all pending messages for the specified client.
        return self.execute(
            f"SELECT ID, source_client, Type, Content FROM {Database.MESSAGES} WHERE target_client = :client_id",
            {"client_id": client_id}
        )
