#include "HexObject.hpp"
#include <fstream>
#include <istream>
#include <algorithm>
#include <iterator>

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
    fileSizeInBytes = std::filesystem::file_size(_filePath);
    _data = new char[fileSizeInBytes]();
    std::ifstream file(_filePath, std::ios::binary);
    file.read(_data, fileSizeInBytes);

    return file.gcount();
}

void HexObject::save_file() {
    std::ofstream file(_filePath, std::ios::binary);
    file.write(_data, fileSizeInBytes); 
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

char* HexObject::begin() {
    return _data;
}

char* HexObject::end() {
    return _data + fileSizeInBytes;
}

void HexObject::find_in_file() {

    std::vector<char> pattern = {static_cast<char>(0xFA), 0x05};

    char* it = std::search(this->begin(), this->end(), pattern.begin(), pattern.end());
    
    int offset = 0;

    while (it != this->end()) {
        offset = std::distance(this->begin(), it);
        // _patternIndex.push_back(offset);
        _patternIndex.insert({offset, pattern.size()});
        offset += pattern.size();
        it = std::search(this->begin() + offset, this->end(), pattern.begin(), pattern.end());
    }    
    
}
