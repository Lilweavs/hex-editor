#include <cstdint>
#include <fstream>
#include <filesystem>

#ifndef HEXOBJECT_H
#define HEXOBJECT_H


class HexObject {
public:

    HexObject() { };
    HexObject(std::filesystem::path filePath);

    ~HexObject();

    void set_filepath(std::filesystem::path filePath);
    size_t get_binary_data_from_file();

    void set_byte(int idx, uint8_t byte); 
    
    char* get_ptr_at_index(int idx);
    
    char& at(int idx);
    
private:
    std::filesystem::path _filePath;
    char* _data;
};



#endif // HEXOBJECT_H
