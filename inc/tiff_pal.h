#ifndef __TIFF_PAL_H
#define __TIFF_PAL_H

#include <mutex>

namespace tiff_pal {
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

#endif
