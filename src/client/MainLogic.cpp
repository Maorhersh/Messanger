#include "MainLogic.h"
#include "FileOperations.h"
#include "SocketHandler.h"
#include "FileIO.h"
#include "Communication.h"
#include "Encoder.h"
#include "RSAWrapper.h"
#include "AESWrapper.h"




MainLogic::MainLogic()
    : _fileHandler(std::make_shared<FileOperations>()),
    _socketHandler(std::make_unique<SocketHandler>()),
    _rsaDecryptor(nullptr),
    _fileIO(std::make_unique<FileIO>(_fileHandler)),
    _communication(std::make_unique<Communication>(_socketHandler.get(), _fileHandler))
{
}


MainLogic::~MainLogic() = default;

void MainLogic::clearcurrentError()
{
    currentError.str("");
    currentError.clear();
}


void MainLogic::setError(const std::string& msg)
{
    clearcurrentError();
    currentError << msg;
}



//this fucntion Parses the server configuration from a file.

bool MainLogic::parseServeInfo()
{
    std::string address, port, error;
    if (!_fileIO->parseServeInfo(address, port, error))
    {
        setError(error);
        return false;
    }
    if (!_socketHandler->setSocketInfo(port, address))
    {
        setError("Invalid server address or port.");
        return false;
    }
    return true;
}


// this Reads client(username, UUID, private key and validates & sets them.
bool MainLogic::parseClientInfo()
{
    std::string username, hexUuid, base64PrivateKey, errorMsg;
    if (!_fileIO->parseClientInfo(username, hexUuid, base64PrivateKey, errorMsg))
    {
        setError("error while trying to open / read CLIENT_INFO: " + errorMsg);
        return false;
    }
    if (!validateAndSetClientData(hexUuid, base64PrivateKey))
    {
        return false;
    }
    _self.username = username;
    return true;
}


//this function Stores the client configuration to a file.
bool MainLogic::storeClientInfo()
{
    if (_self.username.empty())
    {
        setError("user name is missing.");
        return false;
    }

    std::string privateKey;
    if (_rsaDecryptor)
        privateKey = _rsaDecryptor->getPrivateKey();

    if (privateKey.empty())
    {
        setError("Private key is missing.");
        return false;
    }

    std::string hexUuid = Encoder::bytesToHex(_self.id.uuid, CLIENT_ID_SIZE);
    std::string error;
    if (!_fileIO->storeClientInfo(hexUuid, _self.username, privateKey, error))
    {
        setError(error);
        return false;
    }
    return true;
}


//this function checks user input correctness
bool MainLogic::clientInputCorrectness(const std::string& username)
{
    if (username.size() >= CLIENT_NAME_SIZE)
    {
        setError("Username is too long.");
        return false;
    }
    if (!std::all_of(username.begin(), username.end(), ::isalnum))
    {
        setError("Username can only contain letters or digits.");
        return false;
    }
    return true;
}

/**
 * This function Validates and sets client data from configuration.
 * Converts the hex UUID to binary and decodes the Base64 encoded private key.
 * Initializes the RSAPrivateWrapper.
 */
bool MainLogic::validateAndSetClientData(const std::string& hexUuid, const std::string& base64PrivateKey)
{
    std::string uuidBin = Encoder::hexToBytes(hexUuid);
    if (uuidBin.size() != CLIENT_ID_SIZE)
    {
        setError("Invalid UUID size in CLIENT_INFO");
        return false;
    }
    std::copy_n(uuidBin.data(), CLIENT_ID_SIZE, _self.id.uuid);

    std::string decodedPrivateKey = Encoder::decode(base64PrivateKey);
    if (decodedPrivateKey.empty())
    {
        setError("Error while trying to decode private key from CLIENT_INFO");
        return false;
    }

    try {
        _rsaDecryptor.reset(new RSAPrivateWrapper(decodedPrivateKey));
    }
    catch (const std::exception& ex) {
        setError("Error while trying to parse private key from CLIENT_INFO: " + std::string(ex.what()));
        return false;
    }
    return true;
}

//This function Initializes RSA keys and retrieves the public key.

bool MainLogic::initializeRSAKeys(std::string& pubKey)
{
    try {
        _rsaDecryptor.reset(new RSAPrivateWrapper());
    }
    catch (const std::exception& ex) {
        setError("RSA Error: " + std::string(ex.what()));
        return false;
    }
    pubKey = _rsaDecryptor->getPublicKey();
    if (pubKey.size() != PUBLIC_KEY_SIZE)
    {
        setError("Public key size is not matching.");
        return false;
    }
    return true;
}


//this function Registers the client in the server.

bool MainLogic::registerUser(const std::string& username)
{
    if (!clientInputCorrectness(username))
        return false;

    std::string pubKey;
    if (!initializeRSAKeys(pubKey))
        return false;

    RESRegistration response;
    std::string errorMsg;
    if (!_communication->sendRegistrationRequest(username, pubKey, response, errorMsg))
    {
        setError(errorMsg);
        return false;
    }

    _self.id = response.payload;
    _self.username = username;
    _self.publicKeySet = true;

    if (!storeClientInfo())
    {
        setError("Failed storeClientInfo after registration.");
        return false;
    }
    return true;
}


//This function Requests the list of clients from the server.
 
bool MainLogic::requestClientsList()
{
    std::vector<Client> tempClients;
    std::string errorMsg;
    if (!_communication->requestAndParseClientsList(_self.id, tempClients, errorMsg))
    {
        setError(errorMsg);
        return false;
    }
    if (tempClients.empty())
    {
        setError("Server has no users registered. Empty Clients list.");
        return false;
    }
    this->clients = tempClients; 
    return true;
}


//this function Requests the public key of a specific client.

bool MainLogic::requestClientPublicKey(const std::string& username)
{
    ClientID clientId;
    PublicKey publicKey;
    std::string errorMsg;
    if (!_communication->requestAndParsePublicKey(_self.id, username, clientId, publicKey, errorMsg))
    {
        setError(errorMsg);
        return false;
    }
    for (auto& client : clients)
    {
        if (client.id == clientId)
        {
            client.publicKey = publicKey;
            client.publicKeySet = true;
            return true;
        }
    }
    setError("Client was not found after fetching public key.");
    return false;
}


//this function Requests pending messages from the server.

bool MainLogic::requestPendingMessages(std::vector<Message>& messages)
{
    clearcurrentError();
    std::string errorMsg;
    if (!_communication->requestAndParsePendingMessages(
        _self.id,
        messages,
        clients,
        _rsaDecryptor.get(),
        [this](const ClientID& clientId, const SymmetricKey& symKey) {
            return setClientSymmetricKey(clientId, symKey);
        },
        errorMsg))
    {
        setError(errorMsg);
        return false;
    }
    return true;
}



//This function Sets the symmetric key for a given client.

bool MainLogic::setClientSymmetricKey(const ClientID& clientID, const SymmetricKey& symmetricKey)
{
    for (auto& client : clients)
    {
        if (client.id == clientID)
        {
            client.symmetricKey = symmetricKey;
            client.symmetricKeySet = true;
            return true;
        }
    }
    return false;
}

//This function is respondible of sending types of messages to the user.
bool MainLogic::sendMessage(const std::string& username, const MSGType type, const std::string& data)
{
    Client client;
    if (!validateAndGetClient(username, client))
        return false;

    // במקרים של MSG_SYMMETRIC_KEY_SEND נדרשת גם העברת המפתח הסימטרי
    const PublicKey* pubKeyPtr = (type == MSG_SYMMETRIC_KEY_SEND ? &client.publicKey : nullptr);
    SymmetricKey symKeyForMessage;
    const SymmetricKey* symKeyPtr = nullptr;

    std::string payload;
    if (type == MSG_SEND_FILE)
    {
        uint8_t* fileContent = nullptr;
        size_t fileSize = 0;
        if (!_fileHandler->readFromFile(data, fileContent, fileSize))
        {
            setError("Failed reading file \"" + data + "\"");
            return false;
        }
        payload.assign(reinterpret_cast<char*>(fileContent), fileSize);
        delete[] fileContent;
        symKeyPtr = &client.symmetricKey;
    }
    else if (type == MSG_SEND_TEXT)
    {
        payload = data;
        symKeyPtr = &client.symmetricKey;
    }
    else if (type == MSG_SYMMETRIC_KEY_SEND)
    {
        AESWrapper aes;
        symKeyForMessage = aes.getKey(); 
        if (!setClientSymmetricKey(client.id, symKeyForMessage))
        {
            setError("Failed storing symmetric key for client " + client.username);
            return false;
        }
        symKeyPtr = &symKeyForMessage;
    }


    std::string errorMsg;


    bool response = _communication->sendAndEncryptMessage( _self.id,client.id,type,payload,pubKeyPtr,symKeyPtr,errorMsg );

    if (!response)
        setError(errorMsg);

    return response;
}



//This function Checks if you  ask for yourself or if a client exist

bool MainLogic::validateAndGetClient(const std::string& username, Client& client)
{
    if (username == _self.username)
    {
        setError("You cant send message to yourself.");
        return false;
    }
    if (!getViaUserName(username, client))
    {
        setError("The user name '" + username + "' has not found.");
        return false;
    }
    return true;
}


 //This function returns the list of user names from the current clients list.

std::vector<std::string> MainLogic::getUsernames() const
{
    std::vector<std::string> userNames;
    for (const auto& client : clients)
        userNames.push_back(client.username);
    return userNames;
}


//This function checks the name of the user name the client gave exist

bool MainLogic::getViaUserName(const std::string& username, Client& client) const
{
    for (const auto& c : clients)
    {
        if (c.username == username)
        {
            client = c;
            return true;
        }
    }
    return false;
}

