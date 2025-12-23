#include "FileIO.h"
#include <fstream>

// Constructs

// FileIO.cpp
FileIO::FileIO(std::shared_ptr<FileOperations> fileHandler)
    : _fileHandler(std::move(fileHandler))
{
}


//this function parses the SERVER_INFO file to get server address and port.

bool FileIO::parseServeInfo(std::string& address, std::string& port, std::string& error)
{
    bool write = false; // open file in read-only mode
    if (!_fileHandler->open(SERVER_INFO, write))
    {
        error = "Couldn't open " + std::string(SERVER_INFO);
        return false;
    }

    std::string info;
    if (!_fileHandler->readLine(info))
    {
        error = "Couldn't read " + std::string(SERVER_INFO);
        _fileHandler->close();
        return false;
    }
    _fileHandler->close();

    // Remove  whitespace
    Encoder::trim(info);

   
    auto pos = info.find(':');
    if (pos == std::string::npos)
    {
        error = std::string(SERVER_INFO) + " is not written properly; ':' separator is missing.";
        return false;
    }

    // Split the string into address and port.
    address = info.substr(0, pos);
    port = info.substr(pos + 1);
    return true;
}

// this function parses the CLIENT_INFO file to return client information.

bool FileIO::parseClientInfo(std::string& username, std::string& hexUuid, std::string& base64PrivateKey, std::string& error)
{
    // Check if CLIENT_INFO exists; create it if missing.
    if (!_fileHandler->open(CLIENT_INFO, false))
    {
        std::ofstream file(CLIENT_INFO);
        if (!file)
        {
            error = "Error while creating " + std::string(CLIENT_INFO);
            return false;
        }
        file.close();

        if (!_fileHandler->open(CLIENT_INFO, false))
        {
            error = "Failed to open newly created file " + std::string(CLIENT_INFO);
            return false;
        }
    }

    // Read username (first line)
    if (!_fileHandler->readLine(username))
    {
        error = "Error while reading username from " + std::string(CLIENT_INFO);
        _fileHandler->close();
        return false;
    }

    // Read UUID (second line)
    if (!_fileHandler->readLine(hexUuid))
    {
        error = "Couldn't read UUID from " + std::string(CLIENT_INFO);
        _fileHandler->close();
        return false;
    }

    // Read the rest of the file as the Base64 encoded private key.
    std::string line;
    base64PrivateKey.clear();
    while (_fileHandler->readLine(line))
    {
        base64PrivateKey += line;
    }

    if (base64PrivateKey.empty())
    {
        error = "Couldn't read private key from " + std::string(CLIENT_INFO);
        _fileHandler->close();
        return false;
    }

    _fileHandler->close();
    return true;
}


// this function Stores the client's information into the CLIENT_INFO file.
bool FileIO::storeClientInfo( const std::string& uuid, const std::string& username, const std::string& privateKey, std::string& error)
{
    if (!_fileHandler->open(CLIENT_INFO, true))
    {
        error = "Couldn't open " + std::string(CLIENT_INFO);
        return false;
    }

    // Write the username on 1st line.
    if (!_fileHandler->writeLine(username))
    {
        error = "Couldn't write username to " + std::string(CLIENT_INFO);
        _fileHandler->close();
        return false;
    }

    // Write  UUID on 2nd line.
    if (!_fileHandler->writeLine(uuid))
    {
        error = "Couldn't write UUID to " + std::string(CLIENT_INFO);
        _fileHandler->close();
        return false;
    }

    // write private key on the third line.
    std::string encodedKey = Encoder::encode(privateKey);
    if (!_fileHandler->write(reinterpret_cast<const uint8_t*>(encodedKey.data()), encodedKey.size()))
    {
        error = "Error writing private key to " + std::string(CLIENT_INFO);
        _fileHandler->close();
        return false;
    }

    _fileHandler->close();
    return true;
}
