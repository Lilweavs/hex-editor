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

    void set_byte(int idx, uint8_t byte); 
    
    std::byte* get_ptr_at_index(int idx);
    
    std::byte& at(int idx);

    std::byte* begin();

    std::byte* end();

    void find_in_file();
    
    std::unordered_map<int, int> _patternIndex;
    
private:
    
    std::filesystem::path _filePath;
    size_t fileSizeInBytes;
    std::byte* _data;
    
};



#endif // HEXOBJECT_H
