#include "Communication.h"
#include "SocketHandler.h"
#include "FileOperations.h"

#define PACKET_SIZE 1024

//constuctor
Communication::Communication(SocketHandler* socketHandler, std::shared_ptr<FileOperations> fileHandler)
    :socketHandler(socketHandler)
    , fileHandler(std::move(fileHandler))
{
}

// validates header correctness according to protocol
bool Communication::validateHeader(const RESHeader& header, const RSPCode expectedCode, std::string& error)
{
    if (header.code == RESPONSE_GENERAL_ERROR)
    {
        error = "response code  " + std::to_string(RESPONSE_GENERAL_ERROR) + " - general error.";
        return false;
    }
    if (header.code != expectedCode)
    {
        error = "Unexpected response code " + std::to_string(header.code) +
            " received. Expected code was " + std::to_string(expectedCode);
        return false;
    }
    size_t expectedSize = 0;
    switch (header.code)
    {
    case RESPONSE_REGISTRATION_SUCSSES:
        expectedSize = sizeof(RESRegistration) - sizeof(RESHeader);
        break;
    case RESPONSE_PUBLIC_KEY:
        expectedSize = sizeof(RESPublicKey) - sizeof(RESHeader);
        break;
    case RESPONSE_MSG_SENT_TO_SERVER:
        expectedSize = sizeof(RESMessageSend) - sizeof(RESHeader);
        break;
    default:
        return true;  // variable payload size.
    }
    if (header.payloadSize != expectedSize)
    {
        error = "Unexpected payload size " + std::to_string(header.payloadSize) +
            ". Expected size was " + std::to_string(expectedSize);
        return false;
    }
    return true;
}

//function to check if the payload is empty
bool Communication::receiveUnknownPayload(const uint8_t* request, size_t reqSize,
    const RSPCode expectedCode,
    uint8_t*& payload, size_t& size, std::string& error)
{
    struct RESHeader response;
    uint8_t buffer[PACKET_SIZE];
    payload = nullptr;
    size = 0;
    if (request == nullptr || reqSize == 0)
    {
        error = "Invalid request was provided";
        return false;
    }
    if (!socketHandler->connect())
    {
        error = "Failed connecting to server on SocketHandler";
        return false;
    }
    if (!socketHandler->send(request, reqSize))
    {
        socketHandler->close();
        error = "Failed sending request to server on SocketHandler";
        return false;
    }
    if (!socketHandler->receive(buffer, sizeof(buffer)))
    {
        error = "Failed receiving response header from server on SocketHandler";
        return false;
    }
    memcpy(&response, buffer, sizeof(RESHeader));
    if (!validateHeader(response, expectedCode, error))
    {
        error = "Received unexpected response code from server on SocketHandler";
        return false;
    }
    if (response.payloadSize == 0)
        return true;  // no payload.
    size = response.payloadSize;
    payload = new uint8_t[size];
    uint8_t* ptr = buffer + sizeof(RESHeader);
    size_t recSize = sizeof(buffer) - sizeof(RESHeader);
    if (recSize > size)
        recSize = size;
    memcpy(payload, ptr, recSize);
    ptr = payload + recSize;
    while (recSize < size)
    {
        size_t toRead = (size - recSize);
        if (toRead > PACKET_SIZE)
            toRead = PACKET_SIZE;
        if (!socketHandler->receive(buffer, toRead))
        {
            error = "Failed receiving payload data from server on SocketHandler";
            delete[] payload;
            payload = nullptr;
            size = 0;
            return false;
        }
        memcpy(ptr, buffer, toRead);
        recSize += toRead;
        ptr += toRead;
    }
    return true;
}
//checks if the users list is valid
bool Communication::requestUsersList(uint8_t*& payload, size_t& payloadSize, const ClientID& clientId, std::string& error)
{
    REQUsersList request(clientId);

    if (!receiveUnknownPayload(reinterpret_cast<const uint8_t*>(&request),
        sizeof(request),
        RESPONSE_USERS_LIST,
        payload,
        payloadSize,
        error))
    {
        return false;
    }

    return true;
}

/**
 * Sends a request to the server for the current users list that are connected,
 * coping the returned payload into a vector of client structures,if there are errors it handles them.
 */
bool Communication::requestAndParseClientsList(const ClientID& self, std::vector<MainLogic::Client>& clients, std::string& error)
{

    REQUsersList request(self);


    uint8_t* payload = nullptr;
    size_t payloadSize = 0;


    if (!receiveUnknownPayload(reinterpret_cast<const uint8_t*>(&request),
        sizeof(request),
        RESPONSE_USERS_LIST,
        payload,
        payloadSize,
        error))
    {
        return false;
    }

    size_t recordSize = sizeof(ClientID) + CLIENT_NAME_SIZE;


    if (payloadSize == 0 || (payloadSize % recordSize) != 0)
    {
        error = "invalid size on the useres list that has been received";
        delete[] payload;
        return false;
    }

    // clean the previous users lisr
    usersList.clear();

    size_t count = payloadSize / recordSize;
    uint8_t* ptr = payload; // point to the begging of the payload

    for (size_t i = 0; i < count; ++i)
    {
        MainLogic::Client c;


        memcpy(&c.id, ptr, sizeof(ClientID));
        ptr += sizeof(ClientID);

        // copy name of user
        char nameBuf[CLIENT_NAME_SIZE] = { 0 };
        memcpy(nameBuf, ptr, CLIENT_NAME_SIZE - 1);
        ptr += CLIENT_NAME_SIZE;


        c.username = nameBuf;

        //add the client to the list
        usersList.push_back(c);
    }

    //update the clients list
    clients = usersList;

    //free the memory
    delete[] payload;
    return true;
}




// Sends a message from current user to another user, encrypt the payload as needed based on message type.
bool Communication::sendAndEncryptMessage(
    const ClientID& selfId,
    const ClientID& targetId,
    const MSGType type,
    const std::string& data,
    const PublicKey* publicKey,
    const SymmetricKey* symmetricKey,
    std::string& error)
{
    std::string encryptedData;

    // Handle text and file messages (using symmetric encryption)
    if (type == MSG_SEND_FILE) {
        if (!symmetricKey) {
            error = "Missing symmetric key.";
            return false;
        }
        AESWrapper aes(*symmetricKey);
        encryptedData = aes.encrypt(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }
    else if (type == MSG_SEND_TEXT) {
        if (!symmetricKey) {
            error = "Missing symmetric key.";
            return false;
        }
        AESWrapper aes(*symmetricKey);
        encryptedData = aes.encrypt(data);
    }
    // Handle symmetric key exchange messages
    else if (type == MSG_SYMMETRIC_KEY_REQUEST) {
        // According to the original logic, a symmetric key request has no payload.
        encryptedData = "";
    }
    else if (type == MSG_SYMMETRIC_KEY_SEND) {
        if (!symmetricKey) {
            error = "Missing symmetric key.";
            return false;
        }
        if (!publicKey) {
            error = "Missing target's public key.";
            return false;
        }
        // Encrypt the symmetric key (raw bytes) using the target's public key.
        RSAPublicWrapper rsa(*publicKey);
        encryptedData = rsa.encrypt(reinterpret_cast<const uint8_t*>(symmetricKey->symmetricKey), SYMMETRIC_KEY_SIZE);
    }
    else {
        error = "Unexpected message type.";
        return false;
    }

    // Build the request packet
    REQSendMessage request(selfId, type);
    request.payloadHeader.clientId = targetId;
    request.payloadHeader.contentSize = encryptedData.size();
    request.header.payloadSize = sizeof(request.payloadHeader) + encryptedData.size();

    size_t totalSize = sizeof(request) + encryptedData.size();
    std::vector<uint8_t> buffer(totalSize);
    memcpy(buffer.data(), &request, sizeof(request));
    if (!encryptedData.empty())
        memcpy(buffer.data() + sizeof(request), encryptedData.data(), encryptedData.size());

    RESMessageSend response;
    bool ok = socketHandler->sendReceive(buffer.data(), totalSize,
        reinterpret_cast<uint8_t*>(&response), sizeof(response));
    if (!ok) {
        error = "Failed sending message.";
        return false;
    }
    if (response.payload.clientId != targetId) {
        error = "Client ID mismatch.";
        return false;
    }
    return true;
}




/**
 * Requests and get the public key for a given username.
 * This function searches the locally stored users list for the provided username.
 * If found, it extracts the corresponding ClientID and then sends a request to obtain
 */
bool Communication::requestAndParsePublicKey(const ClientID& selfId, const std::string& username, ClientID& clientId, PublicKey& publicKey, std::string& error)
{
    // search for the user in a local users list
    bool found = false;
    for (const auto& client : usersList) {
        if (client.username == username) {
            clientId = client.id;
            found = true;
            break;
        }
    }
    if (!found) {
        error = "Username '" + username + "' not found.";
        return false;
    }

    // Send a request for the target user's public key
    uint8_t* payload = nullptr;
    size_t payloadSize = 0;
    if (!requestClientPublicKey(selfId, clientId, payload, payloadSize, error)) {
        return false;
    }

    // Validate payload length (ClientID + PublicKey)
    const size_t EXPECTED_SIZE = sizeof(PublicKey) + sizeof(ClientID);
    if (payloadSize != EXPECTED_SIZE) {
        error = "Invalid public key payload size. Expected " +
            std::to_string(EXPECTED_SIZE) + ", got " +
            std::to_string(payloadSize) + ".";
        delete[] payload;
        return false;
    }

    // copy returned ClientID and PublicKey
    std::memcpy(&clientId, payload, sizeof(ClientID));
    std::memcpy(&publicKey, payload + sizeof(ClientID), sizeof(PublicKey));

    delete[] payload;
    return true;
}


//this function sends a request to retrieve the public key for a specific client.
bool Communication::requestClientPublicKey(const ClientID& selfId, const ClientID& targetClientId, uint8_t*& payload, size_t& payloadSize, std::string& error)
{
    REQPublicKey request(selfId);
    request.payload = targetClientId;
    request.header.payloadSize = sizeof(request.payload);

    if (!receiveUnknownPayload(reinterpret_cast<const uint8_t*>(&request),
        sizeof(request),
        RESPONSE_PUBLIC_KEY,
        payload,
        payloadSize,
        error))
    {
        return false;
    }

    return true;
}


//this function  gets pending messages for the user it sends
bool Communication::requestAndParsePendingMessages(
    const ClientID& selfId,
    std::vector<MainLogic::Message>& messages,
    std::vector<MainLogic::Client>& clients,
    RSAPrivateWrapper* rsaDecryptor,
    std::function<bool(const ClientID&, const SymmetricKey&)> setSymmetricKey,
    std::string& error)
{
    REQMessages request(selfId);
    uint8_t* payload = nullptr;
    size_t payloadSize = 0;

    if (!receiveUnknownPayload(reinterpret_cast<const uint8_t*>(&request),
        sizeof(request),
        RESPONSE_PULL_PENDING_MSGS,
        payload,
        payloadSize,
        error))
    {
        return false;
    }

    if (payloadSize == 0 || payload == nullptr)
    {
        error = "No pending messages available.";
        delete[] payload;
        return false;
    }

    size_t parsedBytes = 0;
    uint8_t* ptr = payload;
    messages.clear();
    while (parsedBytes < payloadSize)
    {
        if (payloadSize - parsedBytes < sizeof(PendingMessage))
        {
            error = "Invalid pending messages payload size.";
            delete[] payload;
            return false;
        }

        PendingMessage pendingMsg;
        memcpy(&pendingMsg, ptr, sizeof(PendingMessage));
        ptr += sizeof(PendingMessage);
        parsedBytes += sizeof(PendingMessage);

        MainLogic::Message message;
        MainLogic::Client senderClient;
        bool foundSender = false;
        for (const auto& client : clients)
        {
            if (memcmp(&client.id, &pendingMsg.clientId, sizeof(ClientID)) == 0)
            {
                message.username = client.username;
                senderClient = client;
                foundSender = true;
                break;
            }
        }

        switch (pendingMsg.messageType)
        {
        case MSG_SYMMETRIC_KEY_REQUEST:
        {
            // No payload to decrypt – just register the request.
            message.content = "Request for symmetric key.";
            messages.push_back(message);
            // Even if header->messageSize is non-zero, we follow the original logic.
            parsedBytes += pendingMsg.messageSize;
            ptr += pendingMsg.messageSize;
            break;
        }
        case MSG_SYMMETRIC_KEY_SEND:
        {
            if (pendingMsg.messageSize == 0)
            {
                error = "Can't decrypt symmetric key. Content length is 0.";
                parsedBytes += pendingMsg.messageSize;
                ptr += pendingMsg.messageSize;
                continue;
            }

            std::string key;
            try {
                key = rsaDecryptor->decrypt(ptr, pendingMsg.messageSize);
            }
            catch (...)
            {
                error = "Failed to decrypt symmetric key.";
                delete[] payload;
                return false;
            }

            if (key.size() != SYMMETRIC_KEY_SIZE)
            {
                error = "Invalid symmetric key size.";
                delete[] payload;
                return false;
            }
            SymmetricKey symKey;
            memcpy(symKey.symmetricKey, key.data(), SYMMETRIC_KEY_SIZE);
            if (foundSender)
            {
                if (setSymmetricKey(senderClient.id, symKey))
                    message.content = "symmetric key received";
                else
                    error = "Failed to store symmetric key for client.";
                messages.push_back(message);
            }
            else {
                error = "Can't store symmetric key, sender unknown.";
            }
            parsedBytes += pendingMsg.messageSize;
            ptr += pendingMsg.messageSize;
            break;
        }
        case MSG_SEND_TEXT:
        case MSG_SEND_FILE:
        {
            if (pendingMsg.messageSize == 0)
            {
                message.content = "Message with no content provided.";
                parsedBytes += pendingMsg.messageSize;
                ptr += pendingMsg.messageSize;
                break;
            }
            message.content = "can't decrypt message"; // Default in case of failure
            if (foundSender && senderClient.symmetricKeySet)
            {
                try {
                    AESWrapper aes(senderClient.symmetricKey);
                    message.content = aes.decrypt(ptr, pendingMsg.messageSize);
                }
                catch (...) {
                    message.content = "Decryption failed.";
                }
            }
            messages.push_back(message);
            parsedBytes += pendingMsg.messageSize;
            ptr += pendingMsg.messageSize;
            break;
        }
        default:
        {
            message.content = ""; // Corrupted or unknown message – do not store.
            break;
        }
        }
    }
    delete[] payload;
    return true;
}


//this function sends a registration request to the server with the provided username and public key.
bool Communication::sendRegistrationRequest(const std::string& username, const std::string& publicKey,
    RESRegistration& response, std::string& error)
{
    REQRegistration request;
    request.header.payloadSize = sizeof(request.payload);

    strcpy_s((char*)request.payload.clientName.name, CLIENT_NAME_SIZE, username.data());

    auto currentPublicKeySize = publicKey.size();
    if (currentPublicKeySize != PUBLIC_KEY_SIZE) {
        error = "Public key size is not matching.";
        return false;
    }
    memcpy(request.payload.clientPublicKey.publicKey, publicKey.data(), PUBLIC_KEY_SIZE);

    if (!socketHandler->sendReceive(reinterpret_cast<uint8_t*>(&request), sizeof(request),
        reinterpret_cast<uint8_t*>(&response), sizeof(response))) {
        error = "Communication with the server has failed in registration process.";
        return false;
    }

    if (!validateHeader(response.header, RESPONSE_REGISTRATION_SUCSSES, error)) {
        return false;
    }

    return true;
}


//This function Sends a generic message to the server and waits for a response.
bool Communication::sendMessage(uint8_t* response, size_t responseSize, const uint8_t* msg, size_t msgSize, std::string& error)
{
    if (!socketHandler->sendReceive(msg, msgSize, response, responseSize))
    {
        error = "server responded with an error";
        return false;
    }
    return true;
}