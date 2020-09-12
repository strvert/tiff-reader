#include "tiff_reader.h"
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <vector>
#include <type_traits>

#include <mutex>

namespace tiff {

namespace tr_impl {
    constexpr const uint32_t INFO_BUF_SIZE = 32;
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

    inline void pix_buffer_lock() {
        pix_buf_mtx.lock();
    }

    inline void pix_buffer_unlock() {
        pix_buf_mtx.unlock();
    }
}
static_assert(tr_impl::INFO_BUF_SIZE >= 16, "INFO_BUF_SIZE must be at least 16 bytes.");

const std::map<tag_t, std::function<bool(const reader&, const tag_entry&, page&)>> reader::tag_procs = {
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
    {tag_t::PLANAR_CONFIGURATION, tag_manager::planar_configuration},
    {tag_t::RESOLUTION_UNIT, tag_manager::resolution_unit},
    {tag_t::COLOR_MAP, tag_manager::color_map},
    {tag_t::IMAGE_DESCRIPTION, tag_manager::image_description},
    {tag_t::SAMPLES_PER_PIXEL, tag_manager::samples_per_pixel},
    {tag_t::DATE_TIME, tag_manager::date_time},
    {tag_t::EXTRA_SAMPLES, tag_manager::extra_samples},
};

void page::print_info() const
{
    printf("Image Width: %d Image Height: %d\n", width, height);
    printf("Bits/Sample: ");
    for (auto& bps: bit_per_samples) {
        printf("%d ", bps);
    }
    printf("\n");
    printf("Compression Scheme: %s\n", to_string(compression));
    printf("Photometric Interpretation: %s\n", to_string(colorspace));
    printf("%ld Strips:\n", strip_offsets.size());
    for (int i = 0; i < strip_offsets.size(); i++) {
        printf("\t%d: [%10d, %10d]\n", i, strip_offsets[i], strip_byte_counts[i]);
    }
    printf("Samples/Pixel: %d\n", sample_per_pixel);
    printf("Rows/Strip: %d\n", rows_per_strip);
    printf("Extra Samples: %d <%s>\n", extra_sample_counts, to_string(extra_sample_type));
    if (description.length() != 0) {
        printf("Description: %s\n", description.c_str());
    }
    if (date_time.length() != 0) {
        printf("Date Time: %s\n", date_time.c_str());
    }
}

color_t page::get_pixel(const uint16_t x, const uint16_t y) const
{
    using namespace tr_impl;

    uint32_t ptr = (y*width + x) * byte_per_pixel;
    uint16_t target_strip = 0;
    uint32_t total_byte = 0;
    for (auto& s: strip_byte_counts) {
        if (total_byte + s > ptr) break;
        total_byte += s;
        target_strip++;
    }

    ptr -= total_byte;

    pix_buffer_lock();
    r.fread_pos(info_buffer, strip_offsets[target_strip] + ptr, byte_per_pixel);

    color_t c;
    uint8_t* c_u8[4] = {&c.r, &c.g, &c.b, &c.a};
    uint8_t i = 0;
    uint8_t start_pos = 0;
    for (auto& b: bit_per_samples) {
        *c_u8[i++] = extract_memory<uint8_t>(info_buffer, start_pos, b);
        start_pos += b;
    }
    pix_buffer_unlock();

    return c;
}

reader::reader(const std::string& path) :
    path(path)
{
    using namespace tr_impl;
    source = fopen(path.c_str(), "rb");
    if (!source) {
        return;
    }
    if (!read_header()) {
        fclose(source);
        source = 0;
        return;
    }
    decode();
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
    return source && decoded;
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

bool reader::read_entry_tags(const std::vector<ifd> &ifds, std::vector<page> &pages)
{
    uint32_t page_index = 0;
    for (auto& ifd: ifds) {
        for(auto& e: ifd.entries) {
            if (tag_procs.count(e.tag)) {
                if(!tag_procs.at(e.tag)(*this, e, pages[page_index])) {
                    printf("Tag %s(", to_string(e.tag));
                    printf("0x%04X) process failed.\n", enum_base_cast(e.tag));
                    return false;
                }
            } else {
                // printf("Tags id: 0x%04X is not implemented.\n", enum_base_cast(e.tag));
            }
        }
        page_index++;
    }
    return true;
}

bool reader::decode()
{
    fetch_ifds(ifds);
    for (auto& i: ifds) {
        pages.push_back(page(*this));
    }

    if(!read_entry_tags(ifds, pages)) {
        return false;
    }

    decoded = true;
    return true;
}

const page& reader::get_page(uint32_t index) &
{
    return pages[index];
}

uint32_t reader::get_page_count() const
{
    return pages.size();
}

void reader::print_header() const
{
    printf("order: %.2s\n", h.order);
    printf("version: %d\n", h.version);
    printf("offset: %d\n", h.offset);
}

bool reader::tag_manager::image_width(const reader &r, const tag_entry &e, page& p)
{
    r.print_header();
    p.width = read_scalar_generic(r, e);
    return true;
}
bool reader::tag_manager::image_length(const reader &r, const tag_entry &e, page& p)
{
    p.height = read_scalar_generic(r, e);
    return true;
}
bool reader::tag_manager::bits_per_sample(const reader &r, const tag_entry &e, page& p)
{
    using namespace tr_impl;

    if (e.data_field <= 0) return false;
    if (e.field_count * sizeof(uint16_t) > sizeof(uint32_t)) {
        p.bit_per_samples.resize(e.field_count);
        uint32_t ptr = read_scalar<uint32_t>(r, e);
        info_buffer_lock();
        r.fread_array_buffering(p.bit_per_samples, info_buffer, INFO_BUF_SIZE, ptr);
        info_buffer_unlock();
    } else {
        p.bit_per_samples.resize(1);
        p.bit_per_samples[0] = read_scalar<uint16_t>(r, e);
    }

    uint16_t total_bits = 0;
    for (auto& b: p.bit_per_samples) {
        total_bits += b;
    }
    if (total_bits % 8 != 0) return false;
    p.byte_per_pixel = total_bits / 8;
    return true;
}
bool reader::tag_manager::compression(const reader &r, const tag_entry &e, page& p)
{
    auto c = static_cast<compression_t>(read_scalar<uint16_t>(r, e));
    if (c != compression_t::NONE) {
        printf("Compressed tiff is not supported.\n");
        return false;
    }
    p.compression = c;
    return true;
}
bool reader::tag_manager::photometric_interpretation(const reader &r, const tag_entry &e, page& p)
{
    p.colorspace = static_cast<colorspace_t>(read_scalar<uint16_t>(r, e));
    if (p.colorspace == colorspace_t::MINISBLACK);
    switch (p.colorspace) {
    case colorspace_t::MINISBLACK:
    case colorspace_t::MINISWHITE:
    case colorspace_t::RGB:
    case colorspace_t::PALETTE:
    case colorspace_t::MASK:
        return true;
    default:
        printf("Specified color space is not available.\n");
        return false;
    }
}
bool reader::tag_manager::strip_offsets(const reader &r, const tag_entry &e, page& p)
{
    using namespace tr_impl;

    if (e.data_field <= 0) return false;
    p.strip_offsets.resize(e.field_count);
    if (e.field_count >= 2) {
        uint32_t ptr = read_scalar<uint32_t>(r, e);
        info_buffer_lock();
        r.fread_array_buffering(p.strip_offsets, info_buffer, INFO_BUF_SIZE, ptr);
        info_buffer_unlock();
    } else {
        p.strip_offsets[0] = read_scalar<uint32_t>(r, e);
    }
    return true;
}
bool reader::tag_manager::rows_per_strip(const reader &r, const tag_entry &e, page& p)
{
    p.rows_per_strip = read_scalar_generic(r, e);
    return true;
}
bool reader::tag_manager::strip_byte_counts(const reader &r, const tag_entry &e, page& p)
{
    using namespace tr_impl;

    if (e.data_field <= 0) return false;
    p.strip_byte_counts.resize(e.field_count);
    if (e.field_count >= 2) {
        uint32_t ptr = read_scalar<uint32_t>(r, e);
        info_buffer_lock();
        r.fread_array_buffering(p.strip_byte_counts, info_buffer, INFO_BUF_SIZE, ptr);
        info_buffer_unlock();
    } else {
        p.strip_byte_counts[0] = read_scalar<uint32_t>(r, e);
    }
    return true;
}
bool reader::tag_manager::x_resolution(const reader&, const tag_entry&, page&)
{
    // Not yet implemented
    return true;
}
bool reader::tag_manager::y_resolution(const reader&, const tag_entry&, page&)
{
    // Not yet implemented
    return true;
}
bool reader::tag_manager::planar_configuration(const reader &r, const tag_entry &e, page &p)
{
    p.planar_configuration = static_cast<planar_configuration_t>(read_scalar<uint16_t>(r, e));
    if (p.planar_configuration != planar_configuration_t::CONTIG) return false;
    return true;
}
bool reader::tag_manager::resolution_unit(const reader&, const tag_entry&, page&)
{
    // Not yet implemented
    return true;
}
bool reader::tag_manager::color_map(const reader &r, const tag_entry &e, page& p)
{
    using namespace tr_impl;

    if (e.field_count <= 0) return true;

    uint32_t ptr = read_scalar<uint32_t>(r, e);
    if (ptr == 0) return false;

    p.color_palette.resize(e.field_count);
    info_buffer_lock();
    r.fread_array_buffering(p.color_palette, info_buffer, INFO_BUF_SIZE, ptr);
    info_buffer_unlock();
    return true;
}
bool reader::tag_manager::image_description(const reader &r, const tag_entry &e, page& p)
{
    using namespace tr_impl;
    if (e.field_count <= 0) return true;

    if (e.field_count <= 4) {
        uint32_t u32_str = read_scalar<uint32_t>(r, e);
        std::string temp_str(reinterpret_cast<char*>(&u32_str), 0, e.field_count-1);
        p.description.swap(temp_str);
    } else {
        std::vector<uint8_t> temp_vec(e.field_count, 0);
        uint32_t ptr = read_scalar<uint32_t>(r, e);
        info_buffer_lock();
        r.fread_array_buffering(temp_vec, info_buffer, INFO_BUF_SIZE, ptr);
        info_buffer_unlock();
        std::string temp_str(temp_vec.begin(), temp_vec.end()-1);
        p.description.swap(temp_str);
    }
    return true;
}
bool reader::tag_manager::samples_per_pixel(const reader &r, const tag_entry &e, page& p)
{
    p.sample_per_pixel = read_scalar_generic(r, e);
    return true;
}
bool reader::tag_manager::date_time(const reader &r, const tag_entry &e, page& p)
{
    using namespace tr_impl;
    if (e.field_count != 20) return false;

    std::vector<uint8_t> temp_vec(20, 0);
    uint32_t ptr = read_scalar<uint32_t>(r, e);
    info_buffer_lock();
    r.fread_array_buffering(temp_vec, info_buffer, INFO_BUF_SIZE, ptr);
    info_buffer_unlock();
    std::string temp_str(temp_vec.begin(), temp_vec.end()-1);
    p.date_time.swap(temp_str);
    return true;
}
bool reader::tag_manager::extra_samples(const reader &r, const tag_entry &e, page& p)
{
    p.extra_sample_counts = e.field_count;
    p.extra_sample_type = static_cast<extra_data_t>(read_scalar<uint16_t>(r, e));
    return true;
}

}
