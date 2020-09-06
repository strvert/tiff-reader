#ifndef __TIFF_READER_H
#define __TIFF_READER_H

#include <cstdint>
#include <string>

struct tiff_header {
    char order[2];
    uint16_t version;
    uint32_t offset;
};

class tiff_reader {
private:
    const std::string path;
    intptr_t source;

    tiff_header h;

private:
    tiff_reader(const std::string& path);

    void read_header();
    static bool valid_header(const tiff_header& h);

public:
    ~tiff_reader();
    static tiff_reader open(const std::string& path);

    bool is_valid();
    void print_header();

};

#endif
