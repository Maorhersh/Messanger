
#pragma once
#include <string>
#include <cstdint>
#include <base64.h>                   
#include <boost/algorithm/hex.hpp>    
#include <boost/algorithm/string/trim.hpp> 
#include <chrono>
#include <sstream>
#include <exception>
//this class repsonsible for string operations - encoding and decoding
class Encoder
{

    std::string* str = nullptr;

public:

    static void trim(std::string& stringToTrim);
    static std::string getTimestamp();
    static std::string bytesToHex(const uint8_t* buffer, const size_t size);
    static std::string hexToBytes(const std::string& hexString);
    static std::string encode(const std::string& str);
    static std::string decode(const std::string& str);
};
