//this file contains all the client logic functions

#ifndef MainLogic_H
#define MainLogic_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cctype>
#include "protocol.h"       
#include "RSAWrapper.h"    
#include "AESWrapper.h"    

class FileOperations;
class SocketHandler;
class RSAPrivateWrapper;
class FileIO;
class Communication;

class MainLogic {
public:
    struct Client {
        ClientID id;
        std::string username;
        PublicKey publicKey;
        bool publicKeySet = false;
        SymmetricKey symmetricKey;
        bool symmetricKeySet = false;
    };

    struct Message {
        std::string username;
        std::string content;
    };

    MainLogic();
    virtual ~MainLogic();

    // Delete copy and move operations
    MainLogic(const MainLogic&) = delete;
    MainLogic(MainLogic&&) noexcept = delete;
    MainLogic& operator=(const MainLogic&) = delete;
    MainLogic& operator=(MainLogic&&) noexcept = delete;
    // Initialization and configuration
    bool parseServeInfo();
    bool parseClientInfo();
    bool storeClientInfo();
    bool initializeRSAKeys(std::string& pubKey);
    // Client registration and communication
    bool registerUser(const std::string& username);
    bool requestClientsList();
    bool validateAndSetClientData(const std::string& hexUuid, const std::string& base64PrivateKey);
    bool requestClientPublicKey(const std::string& username);
    bool requestPendingMessages(std::vector<Message>& messages);
    bool sendMessage(const std::string& username, const MSGType type, const std::string& data = "");
    bool validateHeader(const RESHeader& header, const RSPCode expectedCode);
    bool setClientSymmetricKey(const ClientID& clientID, const SymmetricKey& symmetricKey);
    bool clientInputCorrectness(const std::string& username);

    // Client management
    std::vector<std::string> getUsernames() const;
    bool getViaUserName(const std::string& username, Client& client) const;
    bool validateAndGetClient(const std::string& username, Client& client);

    // Error handling and self data
    std::string getCurrentError() const { return currentError.str(); }
    std::string getSelfUsername() const { return _self.username; }
    ClientID getSelfClientID() const { return _self.id; }

private:
    void clearcurrentError();
    void setError(const std::string& msg);

private:
    Client _self;
    std::vector<Client> clients;
    std::stringstream currentError;

    std::shared_ptr<FileOperations> _fileHandler;
    std::unique_ptr<SocketHandler> _socketHandler;
    std::unique_ptr<RSAPrivateWrapper> _rsaDecryptor;
    std::unique_ptr<FileIO> _fileIO;
    std::unique_ptr<Communication> _communication;
};

#endif 
