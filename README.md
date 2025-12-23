# MessageU – Secure Client-Server Messaging System

MessageU is an instant messaging system similar to WhatsApp or Facebook Messenger, implemented as part of an academic assignment.  
The system is based on a **client-server architecture** and supports **end-to-end encrypted communication** between multiple users.

The **server** is written in **Python 3**, while the **client** is written in **C++** and runs as a **console application**.  
Messages are always sent through the server, but the server cannot decrypt message contents.

---

## Architecture Overview

MessageU is implemented using a **client-server model**:

- Clients send requests to the server
- The server stores users and messages
- Clients periodically **pull** pending messages from the server
- Supports communication with both **online** and **offline** users
- End-to-End Encryption (E2EE):  
  - Messages are encrypted on the client side  
  - Only the destination client can decrypt them  
  - The server acts only as a message relay

---

## Project Structure

src/
├── server/
│ ├── *.py # Python server source files
│
├── client/
│ ├── *.cpp # C++ client source files
│ ├── *.h # C++ header files



## Technologies Used

### Server
- Python 3
- Standard Python libraries only
- TCP sockets
- Multi-client support using threads and/or selectors
- Stateless protocol handling
- Optional SQL persistence (bonus)

### Client
- C++
- Object-Oriented Programming (OOP)
- Console-based user interface
- Winsock / Boost for networking
- Crypto++ library for encryption

### Database (Bonus)
- SQL database file: `db.defensive`
- Used for persistent storage of clients and messages

---

## Encryption

MessageU implements **End-to-End Encryption (E2EE)**:

### Asymmetric Encryption
- RSA
- 1024-bit keys
- Used for exchanging symmetric keys
- Public keys are stored on the server
- Private keys never leave the client

### Symmetric Encryption
- AES-128
- CBC mode
- IV initialized to zero (as required by the assignment)
- Used for encrypting text messages and files

---

## Server Responsibilities

- Manage registered users
- Store encrypted messages
- Forward messages to clients upon request
- Support multiple concurrent clients
- Remain **stateless** between requests

### Server Startup
- Reads port number from file:
  - `info.myport`
  - Located in the server directory
- If the file does not exist, the server uses default port **1357**

---

## SQL Storage (Bonus Implementation)

When the SQL bonus is enabled, the server stores data in an SQL database file:

- File name: `db.defensive`

### Tables

#### `clients`
| Field | Description |
|------|------------|
| ID | 128-bit unique client identifier |
| UserName | ASCII username (null-terminated) |
| PublicKey | Client public key |
| LastSeen | Timestamp of last client request |

#### `messages`
| Field | Description |
|------|------------|
| ID | Message ID |
| ToClient | Receiver UUID |
| FromClient | Sender UUID |
| Type | Message type |
| Content | Encrypted message content |

- Database file and tables are created automatically if missing

--

## Client Responsibilities

- Register a new user
- Request list of registered users
- Request public keys of other clients
- Send encrypted messages
- Pull pending messages from the server
- Manage symmetric keys per user

### Client Menu
MessageU client at your service.
110) Register
120) Request for clients list
130) Request for public key
140) Request for waiting messages
150) Send a text message
151) Send a request for symmetric key
152) Send your symmetric key
0) Exit client



-

## Client Configuration Files

### Server Address
- File name: `info.server`
- Location: same directory as the client executable
- Format:



### Client Identity
- File name: `info.me`
- Created after successful registration
- Format:
1. Username  
2. UUID (hexadecimal ASCII representation)  
3. Private key (Base64)

---

## Communication Protocol

- Binary protocol over TCP
- Little-endian byte order
- Stateless request-response model
- Supports:
  - Registration
  - Client list request
  - Public key request
  - Message sending
  - Pulling pending messages

The protocol is **strictly defined** and was implemented without modifications.

---

## Error Handling

- Server returns a general error response on failure
- Client prints:

- Program continues to run and waits for the next command

---

## Build and Run

### Server
```bash
cd src/server
python main.py

