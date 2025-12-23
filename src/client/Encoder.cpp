
#include "Encoder.h"


//This function Trims leading and trailing whitespace from the string
void Encoder::trim(std::string& stringToTrim)
{
    boost::algorithm::trim(stringToTrim);
}


//This function reurnes the current time in milliseconds 

std::string Encoder::getTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return std::to_string(ms);
}


//This function converts a byte buffer to  hexadecimal string version.

std::string Encoder::bytesToHex(const uint8_t* buffer, const size_t size)
{
    if (size == 0 || buffer == nullptr)
        return "";

    // Construct a string from the byte buffer.
    const std::string byteString(buffer, buffer + size);
    try
    {
        return boost::algorithm::hex(byteString);
    }
    catch (const std::exception&)
    {
        return "";
    }
}


//This function converts a hexadecimal string to   byte string version.

std::string Encoder::hexToBytes(const std::string& hexString)
{
    if (hexString.empty())
        return "";
    try
    {
        return boost::algorithm::unhex(hexString);
    }
    catch (const std::exception&)
    {
        return "";
    }
}


//This function Encodes the given string into Base64 format.

std::string Encoder::encode(const std::string& input)
{
    std::string encoded;
    // Create a StringSink for  capturing the encoded output.
    CryptoPP::StringSink* sink = new CryptoPP::StringSink(encoded);
    CryptoPP::Base64Encoder* encoder = new CryptoPP::Base64Encoder(sink, false);
   
    CryptoPP::StringSource ss(input, true, encoder);
    return encoded;
}


//This function uses CryptoPP's Base64Decoder to convert the input string.

std::string Encoder::decode(const std::string& input)
{
    std::string decoded;
    CryptoPP::StringSink* sink = new CryptoPP::StringSink(decoded);
    CryptoPP::Base64Decoder* decoder = new CryptoPP::Base64Decoder(sink);
    CryptoPP::StringSource ss(input, true, decoder);
    return decoded;
}
