#include "FileOperations.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <cstdio>
#include <vector>
#include <boost/filesystem.hpp>

// Constructor
FileOperations::FileOperations() : fileIsOpen(false) {}

// Destructor
FileOperations::~FileOperations()
{
    close();
}

//This function Opens a file for reading / writing. and  Creates directories if there is a need.
bool FileOperations::open(const std::string& filePath, bool write)
{
    if (filePath.empty())
        return false;

    std::ios_base::openmode mode;
    if (write)
    {
        mode = std::ios::binary | std::ios::out;
    }
    else
    {
        mode = std::ios::binary | std::ios::in;
    }


    try
    {
        close();

        // Create directories if needed.
        boost::filesystem::path parent = boost::filesystem::path(filePath).parent_path();
        if (!parent.empty())
            boost::filesystem::create_directories(parent);

        fileStream.reset(new std::fstream(filePath, mode));
        fileIsOpen = fileStream->is_open();
    }
    catch (const std::exception&)
    {
        fileIsOpen = false;
    }
    return fileIsOpen;
}

//this function closes the file 
void FileOperations::close()
{
    if (fileStream)
    {
        try
        {
            fileStream->close();
        }
        catch (const std::exception&)
        {
        }
        fileStream.reset();
    }
    fileIsOpen = false;
}

//This function removes the specified file.
bool FileOperations::remove(const std::string& filePath) const
{
    try
    {
        return boost::filesystem::remove(filePath);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// this function returns the system temporary folder.
std::string FileOperations::getTempFolder() const
{
    return boost::filesystem::temp_directory_path().string();
}

// this funciton returns the file size in bytes.
size_t FileOperations::size(const std::string& filePath) const
{
    if (!fileStream || !fileIsOpen)
        return 0;
    try
    {
        boost::filesystem::path fullPath = boost::filesystem::current_path() / filePath;
        return boost::filesystem::file_size(fullPath);
    }
    catch (const boost::filesystem::filesystem_error&)
    {
        return 0;
    }
}

//This function read a number of bytes into the destination.
bool FileOperations::read(uint8_t* const dest, const size_t bytes) const
{
    if (!fileStream || !fileIsOpen || dest == nullptr || bytes == 0)
        return false;
    try
    {
        fileStream->read(reinterpret_cast<char*>(dest), bytes);
        return fileStream->gcount() == static_cast<std::streamsize>(bytes);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// This function writes a number of bytes from the source.
bool FileOperations::write(const uint8_t* const src, const size_t bytes) const
{
    if (!fileStream || !fileIsOpen || src == nullptr || bytes == 0)
        return false;
    try
    {
        fileStream->write(reinterpret_cast<const char*>(src), bytes);
        return !fileStream->fail();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// This function reads line from the file.
bool FileOperations::readLine(std::string& line) const
{
    if (!fileStream || !fileIsOpen)
        return false;
    try
    {
        return static_cast<bool>(std::getline(*fileStream, line));
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// This function writes a line to the file.
bool FileOperations::writeLine(const std::string& line) const
{
    std::string newline = line;
    newline.push_back('\n');
    return write(reinterpret_cast<const uint8_t*>(newline.data()), newline.size());
}

// This function read all of the file content into a buffer.
bool FileOperations::readFromFile(const std::string& filePath, uint8_t*& file, size_t& bytes)
{
    bool write = false;
    if (!open(filePath, write))
        return false;

    bytes = size(filePath);
    if (bytes == 0)
    {
        close();
        return false;
    }

    file = new uint8_t[bytes];

    if (!read(file, bytes))
    {
        delete[] file;
        file = nullptr;
        close();
        return false;
    }

    close();
    return true;
}

// This function writes data to the file.
bool FileOperations::writeToFile(const std::string& filePath, const std::string& data)
{
    if (data.empty())
        return false;

    if (!open(filePath, true))
        return false;

    bool booleanResponse = write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
    close();
    return booleanResponse;
}
