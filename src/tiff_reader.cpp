#include "tiff_reader.h"
#include <cstring>
#include <algorithm>
#include <byteswap.h>

#include <iostream>
#include <mutex>
namespace tr_impl {
    constexpr const uint32_t BUF_SIZE = 6;
    static uint8_t buffer[BUF_SIZE];
    std::mutex buf_mtx;

    inline intptr_t fopen(const char* path, const char* mode) {
        return reinterpret_cast<intptr_t>(::fopen(path, mode));
    }

    inline int fclose(intptr_t file) {
        if (!file) { return EOF; }
        return ::fclose(reinterpret_cast<FILE*>(file));
    }

    inline size_t fread(void* buf, size_t size, size_t n, intptr_t fp) {
        return ::fread(buf, size, n, reinterpret_cast<FILE*>(fp));
    }

    inline void buffer_lock() {
        buf_mtx.lock();
    }

    inline void buffer_unlock() {
        buf_mtx.unlock();
    }
}

tiff_reader::tiff_reader(const std::string& path) : path(path)
{
    source = tr_impl::fopen(path.c_str(), "rb");
    if (!source) {
        return;
    }
    read_header();
    if(!valid_header(h)) {
        tr_impl::fclose(source);
        source = 0;
    }
}

tiff_reader::~tiff_reader()
{
    tr_impl::fclose(source);
}

tiff_reader tiff_reader::open(const std::string& path)
{
    return tiff_reader(path);
}

bool tiff_reader::is_valid()
{
    return source;
}

void tiff_reader::read_header()
{
    tr_impl::fread(&h, sizeof(h), 1, source);
    h.version = __builtin_bswap16(h.version);
    h.offset = __builtin_bswap32(h.offset);
}

bool tiff_reader::valid_header(const tiff_header& h)
{
    bool valid = std::memcmp(h.order, "MM", 2) || std::memcmp(h.order, "II", 2);
    valid &= h.version == 42;

    return valid;
}

void tiff_reader::print_header()
{
    printf("order: %.2s\n", h.order);
    printf("version: %d\n", h.version);
    printf("offset: %d\n", h.offset);
}
