#ifndef __TIFF_READER_H
#define __TIFF_READER_H

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <functional>
#include <type_traits>
#include <vector>
#include <map>
#include <byteswap.h>

#include <iostream>

namespace tiff {

template<typename T>
const std::map<T, const char*> enum_string_map = {};

enum class data_t : uint16_t
{
    BYTE,
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
const std::map<data_t, const char*> enum_string_map<data_t> = {
    {data_t::BYTE, "BYTE"},
    {data_t::ASCII, "ASCII"},
    {data_t::SHORT, "SHORT"},
    {data_t::LONG, "LONG"},
    {data_t::RATIONAL, "RATIONAL"},
    {data_t::SBYTE, "SBYTE"},
    {data_t::UNDEFINED, "UNDEFINED"},
    {data_t::SSHORT, "SSHORT"},
    {data_t::SLONG, "SLONG"},
    {data_t::SRATIONAL, "SRATIONAL"},
    {data_t::FLOAT, "FLOAT"},
    {data_t::DOUBLE, "DOUBLE"}
};

enum class endian_t : uint8_t {
    INVALID,
    BIG,
    LITTLE
};

template<>
const std::map<endian_t, const char*> enum_string_map<endian_t> = {
    {endian_t::INVALID, "INVALID"},
    {endian_t::BIG, "BIG"},
    {endian_t::LITTLE, "LITTLE"}
};

template<typename T>
const char* to_string(T e)
{
    if (enum_string_map<T>.count(e)) {
        return enum_string_map<T>[e];
    }
    return "";
}

struct header {
    char order[2];
    uint16_t version;
    uint32_t offset;
};

struct ifd_entry
{
    uint16_t tag;
    data_t field_type;
    uint32_t field_count;
    uint32_t data_field;
};

struct ifd {
    uint16_t entry_count;
    std::vector<ifd_entry> entries;
    uint32_t next_ifd;
};

class buffer_reader
{
private:
    uint8_t* buf_ptr;
    size_t looking;
    bool need_swap;

public:
    template<typename T, size_t S>
    using check_swappable = typename std::enable_if<sizeof(T)==S && !std::is_pointer<T>::value && std::is_scalar<T>::value, std::true_type>::type;

    // FIXME: I think there are other ways to write it besides the C-style cast.
    template<typename T>
    static auto bswap(const T& v)
        ->typename std::enable_if<check_swappable<T, 1>::value, T>::type
    {
        return v;
    }
    template<typename T>
    static auto bswap(const T& v)
        ->typename std::enable_if<check_swappable<T, 2>::value, T>::type
    {
        return T(__builtin_bswap16(*reinterpret_cast<const uint16_t*>(&v)));
    }
    template<typename T>
    static auto bswap(const T& v)
        ->typename std::enable_if<check_swappable<T, 4>::value, T>::type
    {
        return T(__builtin_bswap32(*reinterpret_cast<const uint32_t*>(&v)));
    }
    template<typename T>
    static auto bswap(const T& v)
        ->typename std::enable_if<check_swappable<T, 8>::value, T>::type
    {
        return T(__builtin_bswap64(*reinterpret_cast<const uint64_t*>(v)));
    }

    template<typename T>
    static T read_by_pos(uint8_t* buf, size_t p)
    {
        T ret;
        std::memcpy(&ret, buf+p, sizeof(T));
        return ret;
    }

    void set_swap_mode(bool b)
    {
        need_swap = b;
    }

public:
    buffer_reader(uint8_t* b, bool need_swap = false) : buf_ptr(b), looking(0), need_swap(need_swap) {}

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

    void seek_top()
    {
        looking = 0;
    }
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

public:
    ~reader();
    static reader open(const std::string& path);

    bool is_valid() const;
    bool is_big_endian() const;
    bool is_little_endian() const;
    void fetch_ifds(std::vector<ifd> &ifds) const;
    reader& decode();

    void print_header() const;
    void print_info() const;

public:
    static const std::map<uint16_t, std::function<void()>> tag_procs;

    static void image_width_tag();
    static void image_length_tag();
    static void bits_per_sample_tag();
    static void compression_tag();
    static void photometric_interpretation_tag();
    static void strip_offsets_tag();
    static void rows_per_strip_tag();
    static void strip_byte_counts_tag();
    static void x_resolution_tag();
    static void y_resolution_tag();
    static void resolution_unit_tag();
    static void color_map_tag();
    static void image_description_tag();
    static void samples_per_pixel_tag();
    static void date_time_tag();
    static void extra_samples_tag();
};

}

#endif
