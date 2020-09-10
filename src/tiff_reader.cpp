#include "tiff_reader.h"
#include <bits/stdint-uintn.h>
#include <bitset>
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

const std::map<tag_t, std::function<bool(reader&, const tag_entry&)>> reader::tag_procs = {
    {tag_t::IMAGE_WIDTH, tag_manager::image_width},
    {tag_t::IMAGE_LENGTH, tag_manager::image_length},
    {tag_t::BITS_PER_SAMPLE, tag_manager::bits_per_sample},
    {tag_t::COMPRESSION, tag_manager::compression},
    {tag_t::PHOTOMETRIC_INTERPRETATION, tag_manager::photometric_interpretation},
    {tag_t::STRIP_OFFSETS, tag_manager::strip_offsets},
    {tag_t::ROWS_PER_STRIP, tag_manager::rows_per_strip},
    {tag_t::STRIP_BYTE_COUNTS, tag_manager::strip_byte_counts},
    {tag_t::X_RESOLUTION, tag_manager::x_resolution},
    {tag_t::Y_RESOLUTION, tag_manager::y_resolution},
    {tag_t::RESOLUTION_UNIT, tag_manager::resolution_unit},
    {tag_t::COLOR_MAP, tag_manager::color_map},
    {tag_t::IMAGE_DESCRIPTION, tag_manager::image_description},
    {tag_t::SAMPLES_PER_PIXEL, tag_manager::samples_per_pixel},
    {tag_t::DATE_TIME, tag_manager::date_time},
    {tag_t::EXTRA_SAMPLES, tag_manager::extra_samples},
};

reader::reader(const std::string& path) : path(path), bit_per_samples({1})
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
    } else if (std::memcmp(s, "II", 2) == 0) {
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

void reader::fetch_ifds(std::vector<ifd> &ifds) const
{
    using namespace tr_impl;

    info_buffer_lock();

    // TODO: In rare cases, there may be more multiple IFD.
    ifds.resize(1);
    buffer_reader r(info_buffer, need_swap);
    fread_pos(info_buffer, h.offset, sizeof(ifd::entry_count));
    r.read(ifds[0].entry_count);
    ifds[0].entries.resize(ifds[0].entry_count);

    size_t e_size = 12;
    for (int i = 0; i < ifds[0].entry_count; i++) {
        fread_pos(info_buffer, h.offset + 2 + i * e_size, e_size);
        r.seek_top();
        r.read(ifds[0].entries[i].tag);
        r.read(ifds[0].entries[i].field_type);
        r.read(ifds[0].entries[i].field_count);
        r.read(ifds[0].entries[i].data_field);
    }
    fread_pos(info_buffer, h.offset, sizeof(ifd::next_ifd));
    r.read(ifds[0].next_ifd);

    info_buffer_unlock();
}

bool reader::read_entry_tags(const std::vector<ifd> &ifds)
{
    for(auto& e: ifds[0].entries) {
        if (tag_procs.count(e.tag)) {
            if(!tag_procs.at(e.tag)(*this, e)) {
                return false;
            }
        } else {
            printf("Tags id: 0x%04x is not implemented.\n", enum_base_cast(e.tag));
        }
    }
    return true;
}

reader& reader::decode()
{
    fetch_ifds(ifds);
    if(!read_entry_tags(ifds)) {
        return *this;
    }

    decoded = true;
    return *this;
}

void reader::print_header() const
{
    printf("order: %.2s\n", h.order);
    printf("version: %d\n", h.version);
    printf("offset: %d\n", h.offset);
}

void reader::print_info() const
{
    if (!decoded) {
        printf("You must first call decode()\n");
        return;
    }

    for(auto& e: ifds[0].entries) {
        printf("tag: %s(0x%04x)\n", to_string(e.tag), enum_base_cast(e.tag));
        printf("field type: %s\n", to_string(e.field_type));
        printf("field count: %d\n", e.field_count);
        printf("data field: 0x%04x\n", e.data_field);
        printf("\n");
    }
}

bool reader::tag_manager::image_width(reader &r, const tag_entry &e)
{
    switch (e.field_type) {
        case data_t::SHORT:
            r.width = read_scalar<uint16_t>(r, e);
            break;
        case data_t::LONG:
            r.width = read_scalar<uint32_t>(r, e);
            break;
        default:
            break;
    }
    return true;
}
bool reader::tag_manager::image_length(reader &r, const tag_entry &e)
{
    switch (e.field_type) {
        case data_t::SHORT:
            r.height = read_scalar<uint16_t>(r, e);
            break;
        case data_t::LONG:
            r.height = read_scalar<uint32_t>(r, e);
            break;
        default:
            break;
    }
    return true;
}
bool reader::tag_manager::bits_per_sample(reader &r, const tag_entry &e)
{
    using namespace tr_impl;

    r.bit_per_samples.resize(e.field_count);
    if (e.field_count * sizeof(uint16_t) > sizeof(uint32_t)) {
        size_t ptr = read_scalar<uint32_t>(r, e);
        info_buffer_lock();
        r.fread_pos(info_buffer, ptr, e.field_count * sizeof(uint16_t));
        buffer_reader rd(info_buffer, r.need_swap);
        rd.read_array(r.bit_per_samples);
        info_buffer_unlock();
    } else {
        r.bit_per_samples[0] = read_scalar<uint16_t>(r, e);
    }
    return true;
}
bool reader::tag_manager::compression(reader &r, const tag_entry &e)
{
    std::cout << __FUNCTION__ << std::endl;
    std::cout << read_scalar<uint16_t>(r, e) << std::endl;
    return true;
}
bool reader::tag_manager::photometric_interpretation(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::strip_offsets(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::rows_per_strip(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::strip_byte_counts(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::x_resolution(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::y_resolution(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::resolution_unit(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::color_map(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::image_description(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::samples_per_pixel(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::date_time(reader &r, const tag_entry&)
{
    return true;
}
bool reader::tag_manager::extra_samples(reader &r, const tag_entry&)
{
    return true;
}

}
