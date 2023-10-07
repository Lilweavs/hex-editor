#include "HexObject.hpp"
#include <cstdint>
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
    _data = new std::byte[fileSizeInBytes]();
    std::ifstream file(_filePath, std::ios::binary);
    file.read(reinterpret_cast<char*>(_data), fileSizeInBytes);

    return file.gcount();
}

void HexObject::save_file() {
    std::ofstream file(_filePath, std::ios::binary);
    file.write(reinterpret_cast<char*>(_data), fileSizeInBytes); 
}

std::byte* HexObject::get_ptr_at_index(int idx) {
    return &_data[idx];
}

std::byte& HexObject::at(int idx) {
    return _data[idx];    
}

void HexObject::set_byte(int idx, uint8_t byte) {
    _data[idx] = static_cast<std::byte>(byte);   
} 

std::byte* HexObject::begin() {
    return _data;
}

std::byte* HexObject::end() {
    return _data + fileSizeInBytes;
}

void HexObject::find_pattern(const std::vector<std::byte>& pattern, uint8_t depth) {
    std::byte* it = std::search(begin(), end(), pattern.begin(), pattern.end());
    
    while (it != this->end()) {
        int offset = std::distance(this->begin(), it);
        _patternIndex.insert({offset, {pattern.size(), depth}});
        offset += pattern.size();
        it = std::search(begin() + offset, end(), pattern.begin(), pattern.end());
    }
}

void HexObject::find_in_file(const std::vector<std::vector<std::byte>>& patterns) {
    _patternIndex.clear();
    for (int i = 0; i < patterns.size(); i++) {
        find_pattern(patterns[i], i+1);
    }
 
}
