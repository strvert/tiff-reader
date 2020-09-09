#include "tiff_reader.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <type_traits>

#include <iostream>
#include <mutex>

namespace tiff {

namespace tr_impl {
    constexpr const uint32_t INFO_BUF_SIZE = 16;
    constexpr const uint32_t PIX_BUF_SIZE = 128;
    static uint8_t info_buffer[INFO_BUF_SIZE];
    static uint8_t pix_buffer[PIX_BUF_SIZE];
    std::mutex info_buf_mtx;
    std::mutex pix_buf_mtx;

    inline intptr_t fopen(const char* path, const char* mode) {
        return reinterpret_cast<intptr_t>(::fopen(path, mode));
    }

    inline int fseek(intptr_t fp, long offset, int origin) {
        return ::fseek(reinterpret_cast<FILE*>(fp), offset, origin);
    }

    inline int fclose(intptr_t file) {
        if (!file) { return EOF; }
        return ::fclose(reinterpret_cast<FILE*>(file));
    }

    inline size_t fread(void* buf, size_t size, size_t n, intptr_t fp) {
        return ::fread(buf, size, n, reinterpret_cast<FILE*>(fp));
    }

    inline void info_buffer_lock() {
        info_buf_mtx.lock();
    }

    inline void info_buffer_unlock() {
        info_buf_mtx.unlock();
    }
}
static_assert(tr_impl::INFO_BUF_SIZE >= 16, "INFO_BUF_SIZE must be at least 16 bytes.");

const std::map<uint16_t, std::function<void()>> reader::tag_procs = {
    {0x0100, image_width_tag},
    {0x0101, image_length_tag},
    {0x0102, bits_per_sample_tag},
    {0x0103, compression_tag},
    {0x0106, photometric_interpretation_tag},
    {0x0111, strip_offsets_tag},
    {0x0116, rows_per_strip_tag},
    {0x0117, strip_byte_counts_tag},
    {0x011A, x_resolution_tag},
    {0x011B, y_resolution_tag},
    {0x0128, resolution_unit_tag},
    {0x0140, color_map_tag},
    {0x010E, image_description_tag},
    {0x0115, samples_per_pixel_tag},
    {0x0132, date_time_tag},
    {0x0152, extra_samples_tag},
};

reader::reader(const std::string& path) : path(path)
{
    using namespace tr_impl;
    source = fopen(path.c_str(), "rb");
    if (!source) {
        return;
    }
    if(!read_header()) {
        fclose(source);
        source = 0;
    }
}

reader::~reader()
{
    tr_impl::fclose(source);
}

reader reader::open(const std::string& path)
{
    return reader(path);
}

bool reader::is_valid() const
{
    return source;
}

bool reader::read_header()
{
    using namespace tr_impl;

    info_buffer_lock();
    fread_pos(info_buffer, 0, 8);
    {
        buffer_reader r(info_buffer);
        r.read_array(h.order);
        endian_t t = check_endian_type(h.order);
        if (t == endian_t::INVALID) return false;
        endi = t;
        need_swap = platform_is_big_endian() != is_big_endian();
        r.set_swap_mode(need_swap);
        r.read(h.version);
        r.read(h.offset);

    }

    info_buffer_unlock();

    if (h.version != 42) return false;
    return true;
}

endian_t reader::check_endian_type(const char* const s)
{
    if (std::memcmp(s, "MM", 2) == 0) {
        return endian_t::BIG;
    } else if (std::memcmp(s, "II", 2)) {
        return endian_t::LITTLE;
    } else {
        return endian_t::INVALID;
    }
}

size_t reader::fread_pos(void* dest, const size_t pos, const size_t size) const
{
    using namespace tr_impl;
    fseek(source, pos, SEEK_SET);
    fread(reinterpret_cast<uint8_t*>(dest), size, 1, source);
    return size;
}

bool reader::is_big_endian() const
{
    return endi == endian_t::BIG;
}

bool reader::is_little_endian() const
{
    return endi == endian_t::LITTLE;
}

void reader::fetch_ifds(std::vector<ifd> &ifd_entries) const
{
    using namespace tr_impl;

    uint16_t c;
    info_buffer_lock();
    fread_pos(&c, h.offset, sizeof(ifd::entry_count));

    ifd_entries.emplace_back();

    info_buffer_unlock();
}

void reader::print_header() const
{
    printf("order: %.2s\n", h.order);
    printf("version: %d\n", h.version);
    printf("offset: %d\n", h.offset);
}

void reader::image_width_tag()
{

}
void reader::image_length_tag()
{

}
void reader::bits_per_sample_tag()
{

}
void reader::compression_tag()
{

}
void reader::photometric_interpretation_tag()
{

}
void reader::strip_offsets_tag()
{

}
void reader::rows_per_strip_tag()
{

}
void reader::strip_byte_counts_tag()
{

}
void reader::x_resolution_tag()
{

}
void reader::y_resolution_tag()
{

}
void reader::resolution_unit_tag()
{

}
void reader::color_map_tag()
{

}
void reader::image_description_tag()
{

}
void reader::samples_per_pixel_tag()
{

}
void reader::date_time_tag()
{

}
void reader::extra_samples_tag()
{

}

}
