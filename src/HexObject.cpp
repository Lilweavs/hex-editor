#include "HexObject.hpp"
#include <cstdint>
#include <fstream>
#include <istream>
#include <algorithm>
#include <iterator>

HexObject::HexObject(std::filesystem::path filePath) {
    this->filePath = filePath;
    get_binary_data_from_file();
}

HexObject::~HexObject() {
    delete[] data;
}

void HexObject::set_filepath(std::filesystem::path filePath) {
    this->filePath = filePath;
}

size_t HexObject::get_binary_data_from_file() {
    fileSize = std::filesystem::file_size(filePath);
    data = new std::byte[fileSize]();
    std::ifstream file(filePath, std::ios::binary);
    file.read(reinterpret_cast<char*>(data), fileSize);

    return file.gcount();
}

void HexObject::save_file() {
    std::ofstream file(filePath, std::ios::binary);
    file.write(reinterpret_cast<char*>(data), fileSize); 
}

std::byte* HexObject::get_ptr_at_index(int idx) {
    return &data[idx];
}

std::byte& HexObject::at(int idx) {
    return data[idx];    
}

void HexObject::set_byte(int idx, uint8_t byte) {
    data[idx] = static_cast<std::byte>(byte);   
} 

std::byte* HexObject::begin() {
    return data;
}

std::byte* HexObject::end() {
    return data + fileSize;
}

size_t HexObject::size() {
    return fileSize;
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
