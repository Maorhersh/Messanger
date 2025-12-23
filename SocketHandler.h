
#pragma once
#include <string>
#include <cstdint>
#include <ostream>
#include <boost/asio/ip/tcp.hpp>

using boost::asio::ip::tcp;
using boost::asio::io_context;

const size_t PACKET_SIZE = 1024;  

//This class is responsible for handling socket communication.
class SocketHandler
{
public:

    SocketHandler();
    virtual ~SocketHandler();


    SocketHandler(const SocketHandler& other)                = delete;
    SocketHandler(SocketHandler&& other) noexcept            = delete;
    SocketHandler& operator=(const SocketHandler& other)     = delete;
    SocketHandler& operator=(SocketHandler&& other) noexcept = delete;

    // Friend operators for printing socket information.
    friend std::ostream& operator<<(std::ostream& os, const SocketHandler* socket) {
        if (socket != nullptr)
            os << socket->_address << ':' << socket->_port;
        return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const SocketHandler& socket) {
        return operator<<(os, &socket);
    }


    static bool isValidAddress(const std::string& address);
    static bool isValidPort(const std::string& port);
    bool setSocketInfo( const std::string& port,  const std::string& address);
    bool connect();
    void close();
    bool receive(uint8_t* const buffer, const size_t size) const;
    bool send(const uint8_t* const buffer, const size_t size) const;
    bool sendReceive(const uint8_t* const toSend, const size_t size, uint8_t* const response, const size_t resSize);

private:

    std::string    _address;     
    std::string    _port;         
    io_context*    _ioContext;    
    tcp::resolver* _resolver;    
    tcp::socket*   _socket;      
    bool           bigEndian;    // Flag for big-endian.
    bool           connected;    // conntection indicator


    void swapBytes(uint8_t* const buffer, size_t size) const;
};
