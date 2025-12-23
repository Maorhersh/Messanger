#pragma once
#include <string>
#include <fstream>
#include <memory>  
#include <cstdint>

//This class responsible for file input/output operations.
class FileOperations
{
public:
    FileOperations();
    virtual ~FileOperations();
    FileOperations(const FileOperations& other) = delete;
    FileOperations(FileOperations&& other) noexcept = delete;
    FileOperations& operator=(const FileOperations& other) = delete;
    FileOperations& operator=(FileOperations&& other) noexcept = delete;
    bool open(const std::string& filePath, bool write);
    void close();
    bool read(uint8_t* const dest, const size_t bytes) const;
    bool write(const uint8_t* const src, const size_t bytes) const;
    bool remove(const std::string& filePath) const;
    size_t size(const std::string& filePath) const;
    bool readLine(std::string& line) const;
    bool writeLine(const std::string& line) const;
    bool readFromFile(const std::string& filePath, uint8_t*& file, size_t& bytes);
    bool writeToFile(const std::string& filePath, const std::string& data);

    std::string getTempFolder() const;

private:
    std::unique_ptr<std::fstream> fileStream;  
    bool fileIsOpen;                           
};
