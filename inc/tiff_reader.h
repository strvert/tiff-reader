#ifndef __TIFF_READER_H
#define __TIFF_READER_H

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <functional>
#include <type_traits>
#include <vector>
#include <map>
#include <byteswap.h>

#include <iostream>
#include <bitset>

namespace tiff {

template<typename T>
constexpr auto enum_base_cast(T e)
    -> std::underlying_type_t<T>
{
    return static_cast<std::underlying_type_t<T>>(e);
}

template<typename T>
const std::map<T, const char*> string_map = {};

template<typename T, std::enable_if_t<std::is_enum<T>::value, std::nullptr_t> = nullptr>
const char* to_string(T e)
{
    if (string_map<T>.count(e)) {
        return string_map<T>.at(e);
    }
    return "UNKNOWN";
}

class buffer_reader
{
private:
    const void* buf_ptr;
    size_t looking;
    bool need_swap;

public:
    template<typename T, size_t S>
    using check_swappable = std::enable_if_t<sizeof(T)==S && !std::is_pointer<T>::value && std::is_scalar<T>::value, std::true_type>;

    // FIXME: I think there are other ways to write it besides the C-style cast.
    template<typename T>
    static auto bswap(const T& v)
        -> std::enable_if_t<check_swappable<T, 1>::value, T>
    {
        return v;
    }
    template<typename T>
    static auto bswap(const T& v)
        -> std::enable_if_t<check_swappable<T, 2>::value, T>
    {
        return T(__builtin_bswap16(*reinterpret_cast<const uint16_t*>(&v)));
    }
    template<typename T>
    static auto bswap(const T& v)
        -> std::enable_if_t<check_swappable<T, 4>::value, T>
    {
        return T(__builtin_bswap32(*reinterpret_cast<const uint32_t*>(&v)));
    }
    template<typename T>
    static auto bswap(const T& v)
        -> std::enable_if_t<check_swappable<T, 8>::value, T>
    {
        return T(__builtin_bswap64(*reinterpret_cast<const uint64_t*>(v)));
    }

    template<typename T>
    static T read_by_pos(const void* buf, const size_t p)
    {
        T ret;
        std::memcpy(&ret, static_cast<const uint8_t*>(buf)+p, sizeof(T));
        return ret;
    }

    void set_swap_mode(const bool b)
    {
        need_swap = b;
    }

public:
    buffer_reader(const void* b, bool need_swap = false) : buf_ptr(b), looking(0), need_swap(need_swap) {}

    template<typename T>
    void read(T &value)
    {
        if (need_swap) {
            value = bswap(read_by_pos<T>(buf_ptr, looking));
        } else {
            value = read_by_pos<T>(buf_ptr, looking);
        }
        looking += sizeof(T);
    }

    template<typename T, size_t L>
    void read_array(T (&array)[L])
    {
        for (size_t i = 0; i < L; i++) {
            read(array[i]);
        }
    }

    template<typename T>
    void read_array(std::vector<T>& vec, size_t start, size_t size)
    {
        for (size_t i = start; i < start+size; i++){
            read(vec[i]);
        }
    }

    template<typename T>
    void read_array(std::vector<T>& vec, size_t size)
    {
        read_array(vec, 0, size);
    }

    template<typename T>
    void read_array(std::vector<T>& vec)
    {
        read_array(vec, vec.size());
    }

    void seek_top()
    {
        looking = 0;
    }
};

enum class data_t : uint16_t
{
    BYTE        = 1,
    ASCII,
    SHORT,
    LONG,
    RATIONAL,
    SBYTE,
    UNDEFINED,
    SSHORT,
    SLONG,
    SRATIONAL,
    FLOAT,
    DOUBLE
};

template<>
const std::map<data_t, const char*> string_map<data_t> = {
    {data_t::BYTE,          "BYTE"},
    {data_t::ASCII,         "ASCII"},
    {data_t::SHORT,         "SHORT"},
    {data_t::LONG,          "LONG"},
    {data_t::RATIONAL,      "RATIONAL"},
    {data_t::SBYTE,         "SBYTE"},
    {data_t::UNDEFINED,     "UNDEFINED"},
    {data_t::SSHORT,        "SSHORT"},
    {data_t::SLONG,         "SLONG"},
    {data_t::SRATIONAL,     "SRATIONAL"},
    {data_t::FLOAT,         "FLOAT"},
    {data_t::DOUBLE,        "DOUBLE"}
};

enum class endian_t : uint8_t {
    INVALID,
    BIG,
    LITTLE
};

template<>
const std::map<endian_t, const char*> string_map<endian_t> = {
    {endian_t::INVALID, "INVALID"},
    {endian_t::BIG,     "BIG"},
    {endian_t::LITTLE,  "LITTLE"}
};

enum class tag_t : uint16_t {
    NEW_SUBFILE_TYPE            = 0x00FE,
    IMAGE_WIDTH                 = 0x0100,
    IMAGE_LENGTH                = 0x0101,
    BITS_PER_SAMPLE             = 0x0102,
    COMPRESSION                 = 0x0103,
    PHOTOMETRIC_INTERPRETATION  = 0x0106,
    STRIP_OFFSETS               = 0x0111,
    ROWS_PER_STRIP              = 0x0116,
    STRIP_BYTE_COUNTS           = 0x0117,
    X_RESOLUTION                = 0x011A,
    Y_RESOLUTION                = 0x011B,
    RESOLUTION_UNIT             = 0x0128,
    COLOR_MAP                   = 0x0140,
    IMAGE_DESCRIPTION           = 0x010E,
    SAMPLES_PER_PIXEL           = 0x0115,
    DATE_TIME                   = 0x0132,
    EXTRA_SAMPLES               = 0x0152,
};

template<>
const std::map<tag_t, const char*> string_map<tag_t> = {
    {tag_t::NEW_SUBFILE_TYPE,           "New Subfile Type"},
    {tag_t::IMAGE_WIDTH,                "Image Width"},
    {tag_t::IMAGE_LENGTH,               "Image Length"},
    {tag_t::BITS_PER_SAMPLE,            "Bits/Sample"},
    {tag_t::COMPRESSION,                "Compression"},
    {tag_t::PHOTOMETRIC_INTERPRETATION, "Photometric Interpretation"},
    {tag_t::STRIP_OFFSETS,              "Strip Offsets"},
    {tag_t::ROWS_PER_STRIP,             "Rows/Strip"},
    {tag_t::STRIP_BYTE_COUNTS,          "Strip Byte Counts"},
    {tag_t::X_RESOLUTION,               "X Resolution"},
    {tag_t::Y_RESOLUTION,               "Y Resolution"},
    {tag_t::RESOLUTION_UNIT,            "Resolution Unit"},
    {tag_t::COLOR_MAP,                  "Color Map"},
    {tag_t::IMAGE_DESCRIPTION,          "Image Description"},
    {tag_t::SAMPLES_PER_PIXEL,          "Samples/Pixel"},
    {tag_t::DATE_TIME,                  "Date Time"},
    {tag_t::EXTRA_SAMPLES,              "Extra Samples"},
};

enum class compression_t : uint16_t {
    NONE                = 1,
    CCITTRLE            = 2,
    CCITTFAX3           = 3,
    CCITTFAX4           = 4,
    LZW                 = 5,
    OJPEG               = 6,
    JPEG                = 7,
    DEFLATE             = 8, // zip
    PACKBITS            = 32773,
};

template<>
const std::map<compression_t, const char*> string_map<compression_t> = {
    {compression_t::NONE,       "None"},
    {compression_t::CCITTRLE,   "CCITT modified Huffman RLE"},
    {compression_t::CCITTFAX3,  "CCITT Group 3 fax encoding"},
    {compression_t::CCITTFAX4,  "CCITT Group 4 fax encoding"},
    {compression_t::LZW,        "LZW"},
    {compression_t::OJPEG,      "JPEG ('old-style' JPEG)"},
    {compression_t::JPEG,       "JPEG ('new-style' JPEG)"},
    {compression_t::DEFLATE,    "Deflate ('Adobe-style', 'zip')"}, // zip
    {compression_t::PACKBITS,   "PackBits"},
};

enum class colorspace_t : uint16_t {
    MINISWHITE  = 0,
    MINISBLACK  = 1,
    RGB         = 2,
    PALETTE     = 3,
    MASK        = 4,
    SEPARATED   = 5,
    YCBCR       = 6,
};

template<>
const std::map<colorspace_t, const char*> string_map<colorspace_t> = {
    {colorspace_t::MINISWHITE, "WhiteIsZero"},
    {colorspace_t::MINISBLACK, "BlackIsZero"},
    {colorspace_t::RGB, "RGB"},
    {colorspace_t::PALETTE, "Palette color"},
    {colorspace_t::MASK, "Transparency Mask"},
    {colorspace_t::SEPARATED, "CMYK"},
    {colorspace_t::YCBCR, "YCbCr"},
};

enum class extra_data_t : uint16_t
{
    UNSPECIFIED = 0,
    ASSOCALPHA  = 1,
    UNASSALPHA  = 2,
};

template<>
const std::map<extra_data_t, const char*> string_map<extra_data_t> = {
    {extra_data_t::UNSPECIFIED, "Unspecified data"},
    {extra_data_t::ASSOCALPHA, "Associated alpha data (pre-multiplied color)"},
    {extra_data_t::UNASSALPHA, "Unassociated alpha data"},
};

struct header
{
    char order[2];
    uint16_t version;
    uint32_t offset;
};

struct tag_entry
{
    tag_t tag;
    data_t field_type;
    uint32_t field_count;
    uint32_t data_field;
};

struct ifd
{
    uint16_t entry_count;
    std::vector<tag_entry> entries;
    uint32_t next_ifd;
};

struct rational_t
{
    uint32_t num;
    uint32_t den;
};

class reader {
private:
    const std::string path;
    intptr_t source;

    endian_t endi;
    bool need_swap;

    header h;
    std::vector<ifd> ifds;
    bool decoded = false;

    uint32_t width;
    uint32_t height;
    std::vector<uint16_t> bit_per_samples;
    uint16_t sample_per_pixel;
    compression_t compression;
    colorspace_t colorspace;
    std::vector<uint16_t> color_palette;

    std::vector<uint32_t> strip_offsets;
    uint32_t rows_per_strip;
    std::vector<uint32_t> strip_byte_counts;
    uint32_t extra_sample_counts;
    extra_data_t extra_sample_type;
    rational_t x_resolution;
    rational_t y_resolution;

    std::string description;
    std::string date_time;


private:
    reader(const std::string& path);

    bool read_header();

    inline static bool platform_is_little_endian()
    {
        uint32_t t = 1;
        return *reinterpret_cast<uint8_t*>(&t) == 1;
    }
    inline static bool platform_is_big_endian()
    {
        return !platform_is_little_endian();
    }

    static endian_t check_endian_type(const char s[2]);
    size_t fread_pos(void* dest, const size_t pos, const size_t size) const;
    template<typename T>
    void fread_array_buffering(std::vector<T>& vec, const size_t count, void* buffer, const size_t bufsize, const size_t pos)
    {
        buffer_reader rd(buffer, need_swap);
        uint32_t total = count * sizeof(T);
        uint32_t look = 0;
        while(total-look > 0) {
            uint32_t size
                = std::min(static_cast<uint32_t>((bufsize / sizeof(T))*sizeof(T)), total-look);
            fread_pos(buffer, pos+look, size);
            rd.read_array(vec, look/sizeof(T), size/sizeof(T));
            rd.seek_top();
            look += size;
        }
    }

    template<typename T>
    void fread_array_buffering(std::vector<T>& vec, void* buffer, const size_t bufsize, const size_t pos)
    {
        fread_array_buffering(vec, vec.size(), buffer, bufsize, pos);
    }

public:
    ~reader();
    static reader open(const std::string& path);

    bool is_valid() const;
    bool is_big_endian() const;
    bool is_little_endian() const;
    void fetch_ifds(std::vector<ifd> &ifds) const;
    bool read_entry_tags(const std::vector<ifd> &ifds);
    reader& decode();

    void print_header() const;
    void print_info() const;

public:
    const static std::map<tag_t, std::function<bool(reader&, const tag_entry&)>> tag_procs;

private:
    struct tag_manager
    {
        template<typename T>
        static T read_scalar(const reader& r, const tag_entry &e)
        {
            if (r.need_swap) {
                return e.data_field >> ((sizeof(e.data_field) - sizeof(T)) * 8);
            } else {
                return e.data_field;
            }
        }
        static uint32_t read_scalar_generic(const reader& r, const tag_entry &e)
        {
            switch (e.field_type) {
                case data_t::BYTE:
                    return read_scalar<uint8_t>(r, e);
                    break;
                case data_t::SHORT:
                    return read_scalar<uint16_t>(r, e);
                    break;
                case data_t::LONG:
                    return read_scalar<uint32_t>(r, e);
                    break;
                default:
                    break;
            }
            return 0;
        }

        static bool image_width(reader&, const tag_entry&);
        static bool image_length(reader&, const tag_entry&);
        static bool bits_per_sample(reader&, const tag_entry&);
        static bool compression(reader&, const tag_entry&);
        static bool photometric_interpretation(reader&, const tag_entry&);
        static bool strip_offsets(reader&, const tag_entry&);
        static bool rows_per_strip(reader&, const tag_entry&);
        static bool strip_byte_counts(reader&, const tag_entry&);
        static bool x_resolution(reader&, const tag_entry&);
        static bool y_resolution(reader&, const tag_entry&);
        static bool resolution_unit(reader&, const tag_entry&);
        static bool color_map(reader&, const tag_entry&);
        static bool image_description(reader&, const tag_entry&);
        static bool samples_per_pixel(reader&, const tag_entry&);
        static bool date_time(reader&, const tag_entry&);
        static bool extra_samples(reader&, const tag_entry&);
    };
};

}

#endif
