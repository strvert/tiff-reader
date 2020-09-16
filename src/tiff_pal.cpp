#include "tiff_pal.h"

constexpr const uint32_t tiff_pal::INFO_BUF_SIZE;
constexpr const uint32_t tiff_pal::PIX_BUF_SIZE;
uint8_t tiff_pal::info_buffer[tiff_pal::INFO_BUF_SIZE];
uint8_t tiff_pal::pix_buffer[tiff_pal::PIX_BUF_SIZE];

namespace pal_mtx {
    std::mutex info_buf_mtx;
    std::mutex pix_buf_mtx;
}

intptr_t tiff_pal::fopen(const char* path, const char* mode) {
    return reinterpret_cast<intptr_t>(::fopen(path, mode));
}

int tiff_pal::fseek(intptr_t fp, long offset, int origin) {
    return ::fseek(reinterpret_cast<FILE*>(fp), offset, origin);
}

int tiff_pal::fclose(intptr_t file) {
    if (!file) { return EOF; }
    return ::fclose(reinterpret_cast<FILE*>(file));
}

size_t tiff_pal::fread(void* buf, size_t size, size_t n, intptr_t fp) {
    return ::fread(buf, size, n, reinterpret_cast<FILE*>(fp));
}

void tiff_pal::info_buffer_lock() {
    pal_mtx::info_buf_mtx.lock();
}

void tiff_pal::info_buffer_unlock() {
    pal_mtx::info_buf_mtx.unlock();
}

void tiff_pal::pix_buffer_lock() {
    pal_mtx::pix_buf_mtx.lock();
}

void tiff_pal::pix_buffer_unlock() {
    pal_mtx::pix_buf_mtx.unlock();
}
