#ifndef __TIFF_PAL_H
#define __TIFF_PAL_H

#include <mutex>

struct tiff_pal
{
    struct stat {
        bool in_use = false;
        size_t strip = 0;
        size_t start = 0;
        size_t len = 0;
    };

    constexpr const static uint32_t INFO_BUF_SIZE = 32;
    constexpr const static uint32_t PIX_BUF_SIZE = 128;
    constexpr const static uint32_t PIX_BUF_COUNT = 5;
    static uint8_t info_buffer[INFO_BUF_SIZE];
    static uint8_t pix_buffer[PIX_BUF_COUNT][PIX_BUF_SIZE];
    static stat pix_buffer_statics[PIX_BUF_COUNT];

    static bool init();
    static bool deinit();
    static intptr_t fopen(const char* path, const char* mode);
    static int fseek(intptr_t fp, long offset, int origin);
    static int fclose(intptr_t file);
    static size_t fread(void* buf, size_t size, size_t n, intptr_t fp);
    static void info_buffer_lock();
    static void info_buffer_unlock();
    static void pix_buffer_lock();
    static void pix_buffer_unlock();
};

static_assert(tiff_pal::INFO_BUF_SIZE >= 16, "INFO_BUF_SIZE must be at least 16 bytes.");

#endif
