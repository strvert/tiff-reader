#include "tiff_reader.h"
#include <type_traits>

namespace tr_impl {
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

    }

    static uint8_t buffer[512];
}

tiff_reader::tiff_reader(const std::string& path) : path(path)
{
    source = tr_impl::fopen(path.c_str(), "rb");
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

void tiff_reader::info(tiff_header& h)
{
    tr_impl::fread();
}
