#ifndef Communication_H
#define Communication_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstring>

#include "protocol.h"
#include "MainLogic.h"
#include "FileOperations.h"
#include "RSAWrapper.h"
#include "AESWrapper.h"

class SocketHandler;

/**
 * Responsible for communication between the client and the server.
 */
class Communication {
public:

    Communication(SocketHandler* socketHandler, std::shared_ptr<FileOperations> fileHandler);


    bool sendRegistrationRequest(const std::string& username,
        const std::string& publicKey,
        RESRegistration& response,
        std::string& error);


    bool requestUsersList(uint8_t*& payload,
        size_t& payloadSize,
        const ClientID& clientId,
        std::string& error);

    bool requestClientPublicKey(const ClientID& selfId,
        const ClientID& targetClientId,
        uint8_t*& payload,
        size_t& payloadSize,
        std::string& error);


    bool requestAndParsePublicKey(const ClientID& selfId,
        const std::string& username,
        ClientID& clientId,
        PublicKey& publicKey,
        std::string& error);


    bool requestAndParseClientsList(const ClientID& selfId,
        std::vector<MainLogic::Client>& clients,
        std::string& error);


    bool requestAndParsePendingMessages(const ClientID& selfId,
        std::vector<MainLogic::Message>& messages,
        std::vector<MainLogic::Client>& clients,
        RSAPrivateWrapper* rsaDecryptor,
        std::function<bool(const ClientID&, const SymmetricKey&)> setSymmetricKey,
        std::string& error);


    bool sendAndEncryptMessage(const ClientID& selfId,
        const ClientID& targetId,
        const MSGType type,
        const std::string& data,
        const PublicKey* publicKey,
        const SymmetricKey* symmetricKey,
        std::string& error);


    bool sendMessage(uint8_t* response,
        size_t responseSize,
        const uint8_t* msg,
        size_t msgSize,
        std::string& error);


    bool receiveUnknownPayload(const uint8_t* request,
        size_t reqSize,
        const RSPCode expectedCode,
        uint8_t*& payload,
        size_t& size,
        std::string& error);

private:

    bool sendRequestAndGetPayload(const void* request,
        size_t requestSize,
        RSPCode expectedCode,
        std::vector<uint8_t>& outPayload,
        std::string& error);


    bool validateHeader(const RESHeader& header,
        const RSPCode expectedCode,
        std::string& error);

private:
    SocketHandler* socketHandler;
    std::shared_ptr<FileOperations> fileHandler;

    std::vector<MainLogic::Client> usersList;
};

#endif
