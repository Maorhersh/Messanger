#include "SocketHandler.h"
#include <boost/asio.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/algorithm/string.hpp>
#include <cctype>
#include <algorithm>

using boost::asio::ip::tcp;
using boost::asio::io_context;

// Constructor
SocketHandler::SocketHandler()
    : _ioContext(nullptr)
    , _resolver(nullptr)
    , _socket(nullptr)
    , connected(false)
{
   
    uint32_t endianValue = 1;
    bigEndian = (*reinterpret_cast<uint8_t*>(&endianValue) == 0);
}


SocketHandler::~SocketHandler()
{
    close();
}

// Set address/port after checking validity
bool SocketHandler::setSocketInfo( const std::string& port, const std::string& address)
{
    if (isValidAddress(address) && isValidPort(port))
    {
        _address = address;
        _port = port;
        return true;
    }
    return false;
}

// Validate IPv4 address or "localhost"
bool SocketHandler::isValidAddress(const std::string& address)
{
    if (boost::iequals(address, "localhost"))
        return true;
    try
    {
        boost::asio::ip::address_v4 ipv4Address = boost::asio::ip::make_address_v4(address);
        return true;
    }
    catch (...) 
    {
        return false;
    }
}

// check if the port is positive numeber or missing
bool SocketHandler::isValidPort(const std::string& port)
{
    if (port.empty() || !std::all_of(port.begin(), port.end(), ::isdigit))
        return false;
    return std::stoi(port) > 0;
}

bool SocketHandler::connect()
{
    if (!isValidAddress(_address) || !isValidPort(_port))
        return false;
    try
    {
        close();  // Clean previous connection

        _ioContext = new io_context;
        _resolver = new tcp::resolver(*_ioContext);
        _socket = new tcp::socket(*_ioContext);

        boost::asio::connect(*_socket, _resolver->resolve(_address, _port));
        _socket->non_blocking(false);  
        connected = true;
    }
    catch (...)  
    {
        connected = false;
    }
    return connected;
}

// Close connection and cleanup resources
void SocketHandler::close()
{
    try
    {
        if (_socket != nullptr)
            _socket->close();
    }
    catch (...) {}

    delete _socket;       
    delete _resolver;     
    delete _ioContext;    
    _socket = nullptr;
    _resolver = nullptr;
    _ioContext = nullptr;
    connected = false;
}

//This function Sends data in chunks with endian conversion
bool SocketHandler::send(const uint8_t* const buffer, const size_t size) const
{
    if (_socket == nullptr || !connected || buffer == nullptr || size == 0)
        return false;

    size_t totalBytesSent = 0;
    while (totalBytesSent < size)
    {
        boost::system::error_code errorCode;
        size_t chunkSize = std::min(PACKET_SIZE, size - totalBytesSent);
        uint8_t packet[PACKET_SIZE] = { 0 };
        memcpy(packet, buffer + totalBytesSent, chunkSize);

        // Convert to network byte order if needed
        if (bigEndian)
            swapBytes(packet, chunkSize);

        // WARNING: Incorrect buffer size (should use chunkSize)
        size_t bytesWritten = write(*_socket, boost::asio::buffer(packet, PACKET_SIZE), errorCode);
        if (errorCode || bytesWritten == 0)
            return false;
        totalBytesSent += bytesWritten;
    }
    return true;
}

//This function Receivse data with endian conversion
bool SocketHandler::receive(uint8_t* const buffer, const size_t size) const
{
    
    size_t amountReceived = 0;
    uint8_t tempBuffer[PACKET_SIZE] = { 0 };

    if (!_socket || !connected || buffer == nullptr || size == 0)
        return false;

    while (amountReceived < size)
    {
        boost::system::error_code errorCode;
        size_t bytesRead = read(*_socket, boost::asio::buffer(tempBuffer, PACKET_SIZE), errorCode);
        if (bytesRead == 0)
            return false;

  
        if (bigEndian)
            swapBytes(tempBuffer, bytesRead);

        // Copy to output buffer
        size_t bytesToCopy = std::min(bytesRead, size - amountReceived);
        memcpy(buffer + amountReceived, tempBuffer, bytesToCopy);
        amountReceived += bytesToCopy;
    }
    return true;
}

// This function Checks response and sends/receives data
bool SocketHandler::sendReceive(const uint8_t* const toSend, const size_t size,
    uint8_t* const response, const size_t resSize)
{
    if (!connect())
        return false;

    bool booleanResponse = false;
    if (send(toSend, size))
        booleanResponse = receive(response, resSize);
    close();  // Always close after operation
    return booleanResponse;
}

// This function is for endian conversion it Swaps 32-bit order
void SocketHandler::swapBytes(uint8_t* const buffer, size_t size) const
{
    if (buffer == nullptr || size < sizeof(uint32_t))
        return;

    //  4-byte segment
    const size_t numInts = size / sizeof(uint32_t);
    uint32_t* const ptr = reinterpret_cast<uint32_t*>(buffer);
    for (size_t i = 0; i < numInts; ++i)
    {
        // Reverse the bytes for each 32-bit value
        uint32_t value = ptr[i];
        ptr[i] = ((value & 0xFF000000) >> 24) |
                 ((value & 0x00FF0000) >> 8) |
                 ((value & 0x0000FF00) << 8) |
                 ((value & 0x000000FF) << 24);
    }
}