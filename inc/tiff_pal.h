#ifndef __TIFF_PAL_H
#define __TIFF_PAL_H

#include <mutex>

struct tiff_pal
{
    constexpr const static uint32_t INFO_BUF_SIZE = 32;
    constexpr const static uint32_t PIX_BUF_SIZE = 128;
    static uint8_t info_buffer[INFO_BUF_SIZE];
    static uint8_t pix_buffer[PIX_BUF_SIZE];

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

#endif
