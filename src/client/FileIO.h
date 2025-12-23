
#ifndef FileIO_H
#define FileIO_H

#include "FileOperations.h"
#include "Encoder.h"
#include <sstream>
#include <cstring>
#include <string>
#include "protocol.h"  

class FileOperations;

/**
 * This class is reponsible for file input/output operations for the client configuration.
 */
class FileIO {
public:

    FileIO(std::shared_ptr<FileOperations> fileHandler);


    bool parseServeInfo(std::string& address, std::string& port, std::string& error);


    bool parseClientInfo(std::string& username, std::string& uuid, std::string& privateKey, std::string& error);


    bool storeClientInfo( const std::string& uuid, const std::string& username, const std::string& privateKey, std::string& error);

private:
    std::shared_ptr<FileOperations> _fileHandler;
};

#endif
