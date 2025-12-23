

#pragma once

#include <cstdint>
#include <cstring>  

#define SERVER_INFO "server.info"
#define CLIENT_INFO "me.info"

typedef uint8_t  version_t;
typedef uint16_t code_t;
typedef uint8_t  messageType_t;
typedef uint32_t messageID_t;
typedef uint32_t csize_t;  


const version_t CLIENT_VERSION = 2;
const size_t    CLIENT_ID_SIZE = 16;
const size_t    CLIENT_NAME_SIZE = 255;
const size_t    PUBLIC_KEY_SIZE = 160;  
const size_t    SYMMETRIC_KEY_SIZE = 16;  
const size_t    REQUEST_OPTIONS = 5;
const size_t    RESPONSE_OPTIONS = 6;

//better understaing when using them in functions instead of constans
enum REQCode
{
    REQUEST_REGISTRATION = 600,   
    REQUEST_USERS_LIST = 601,  
    REQUEST_PULL_USER_PUBLIC_KEY = 602,
    REQUEST_SEND_MSG_TO_USER = 603,
    REQUEST_PULL_PENDING_MSGS = 604   
};

enum RSPCode
{
    RESPONSE_REGISTRATION_SUCSSES = 2100,
    RESPONSE_USERS_LIST = 2101,
    RESPONSE_PUBLIC_KEY = 2102,
    RESPONSE_MSG_SENT_TO_SERVER = 2103,
    RESPONSE_PULL_PENDING_MSGS = 2104,
    RESPONSE_GENERAL_ERROR = 9000   
};

enum MSGType
{
    MSG_SYMMETRIC_KEY_REQUEST = 1,  
    MSG_SYMMETRIC_KEY_SEND = 2,  
    MSG_SEND_TEXT = 3,  
    MSG_SEND_FILE = 4   
};


#pragma pack(push, 1)

//client id has 16 bytes, overrided the comperison fucntions 
struct ClientID
{
    uint8_t uuid[CLIENT_ID_SIZE] = { 0 };

    ClientID() = default;

    bool operator==(const ClientID& otherID) const
    {
        for (size_t i = 0; i < CLIENT_ID_SIZE; ++i)
        {
            if (uuid[i] != otherID.uuid[i])
                return false;
        }
        return true;
    }

    bool operator!=(const ClientID& otherID) const
    {
        return !(*this == otherID);
    }
};

struct ClientName
{
    uint8_t name[CLIENT_NAME_SIZE] = { '\0' };
};


struct PublicKey
{
    uint8_t publicKey[PUBLIC_KEY_SIZE] = {};
};

struct SymmetricKey
{
    uint8_t symmetricKey[SYMMETRIC_KEY_SIZE] = {};
};


//seperated struct for request and response for each kind of each type of request and response so it will be easier to manahe in the functions
struct REQHeader
{
    ClientID       clientId;
    const version_t version;    
    const code_t    code;      
    csize_t         payloadSize;

    REQHeader(const code_t reqCode)
        : version(CLIENT_VERSION)
        , code(reqCode)
        , payloadSize(0)
    {
    }

    REQHeader(const ClientID& id, const code_t reqCode)
        : clientId(id)
        , version(CLIENT_VERSION)
        , code(reqCode)
        , payloadSize(0)
    {
    }

};

struct RESHeader
{
    version_t version;
    code_t    code;
    csize_t   payloadSize;

    RESHeader() : version(0), code(0), payloadSize(0) {}
};


struct REQRegistration
{
    REQHeader header;
    struct
    {
        ClientName clientName;
        PublicKey  clientPublicKey;
    } payload;

    REQRegistration()
        : header(REQUEST_REGISTRATION)
    {
    }
};

struct RESRegistration
{
    RESHeader header;
    ClientID  payload;
};

struct REQUsersList
{
    REQHeader header;
    REQUsersList(const ClientID& id)
        : header(id, REQUEST_USERS_LIST)
    {
    }
};

struct RESUsersList
{
    RESHeader header;

};

struct REQPublicKey
{
    REQHeader header;
    ClientID  payload;

    REQPublicKey(const ClientID& id)
        : header(id, REQUEST_PULL_USER_PUBLIC_KEY)
    {
    }
};

struct RESPublicKey
{
    RESHeader header;
    struct
    {
        ClientID  clientId;
        PublicKey clientPublicKey;
    } payload;
};

struct REQSendMessage
{
    REQHeader header;

    struct SPayloadHeader
    {
        ClientID           clientId;    // לקוח יעד
        const messageType_t messageType;
        csize_t             contentSize;

        SPayloadHeader(const messageType_t type)
            : clientId{}
            , messageType(type)
            , contentSize(0)
        {
        }
    } payloadHeader;

    REQSendMessage(const ClientID& id, const messageType_t type)
        : header(id, REQUEST_SEND_MSG_TO_USER)
        , payloadHeader(type)
    {
    }
};

struct RESMessageSend
{
    RESHeader header;
    struct SPayload
    {
        ClientID   clientId;    
        messageID_t messageId;

        SPayload()
            : clientId{}
            , messageId(0)
        {
        }
    } payload;
};

struct REQMessages
{
    REQHeader header;
    REQMessages(const ClientID& id)
        : header(id, REQUEST_PULL_PENDING_MSGS)
    {
    }
};

struct PendingMessage
{
    ClientID     clientId;     
    messageID_t  messageId;
    messageType_t messageType;
    csize_t      messageSize;

    PendingMessage()
        : clientId{}
        , messageId(0)
        , messageType(0)
        , messageSize(0)
    {
    }
};

#pragma pack(pop)
