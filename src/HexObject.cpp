#include <HexObject.hpp>
#include <filesystem>
#include <fstream>
#include <istream>
#include <type_traits>


HexObject::HexObject(std::filesystem::path filePath) {
    _filePath = filePath;
    get_binary_data_from_file();
}

HexObject::~HexObject() {
    delete[] _data;
}

void HexObject::set_filepath(std::filesystem::path filePath) {
    _filePath = filePath;
}

size_t HexObject::get_binary_data_from_file() {
    size_t fileSizeInBytes = std::filesystem::file_size(_filePath);
    _data = new char[fileSizeInBytes]();
    std::ifstream file(_filePath, std::ios::binary);
    file.read(_data, fileSizeInBytes);

    return file.gcount();
}

char* HexObject::get_ptr_at_index(int idx) {
    return &_data[idx];
}

char& HexObject::at(int idx) {
    return _data[idx];    
}

void HexObject::set_byte(int idx, uint8_t byte) {
    _data[idx] = byte;   
} 
