#include <cstdint>
#include <fstream>
#include <filesystem>
#include <memory>
#include <vector>
#include <unordered_map>

#ifndef HEXOBJECT_H
#define HEXOBJECT_H


class HexObject {
public:

    HexObject() { };
    
    HexObject(std::filesystem::path filePath);

    ~HexObject();

    void set_filepath(std::filesystem::path filePath);

    void save_file();

    size_t get_binary_data_from_file();

    void set_bytes(int idx, std::vector<int>& selections, std::byte value);

    void set_bytes(const int idx, const std::vector<std::byte>& bytes); 
    
    std::byte* get_ptr_at_index(int idx);
    
    std::byte& at(int idx);

    std::byte* begin();

    std::byte* end();

    size_t size();

    void find_in_file(const std::vector<std::vector<std::byte>>& patterns);
    
    void find_pattern(const std::vector<std::byte>& pattern, uint8_t depth);
    
    std::unordered_map<int, std::pair<int,uint8_t>> _patternIndex;
    
private:
    
    std::filesystem::path filePath;
    size_t fileSize;
    std::byte* data;
    
};



#endif // HEXOBJECT_H
