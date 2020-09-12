#include <bits/stdint-intn.h>
#include <iostream>
#include <fstream>
#include <string>
#include <type_traits>
#include <thread>

#include "tiff_reader.h"

int main()
{
    std::vector<std::string> imgs = {
        // "./jp.tif",
        // "./p_jp.tif",
        // "./transparent.tiff",
        // "./be.tif",
        // "./high_le.tiff",
        // "./high_be.tiff",
        // "./1MB.tiff",
        // "./1MB_be.tiff",
        "./gray.tif",
        // "./gray_be.tiff",
    };


    for (auto& i: imgs) {
        auto r = tiff::reader::open(i);
        if (!r.is_valid()) {
            std::cout << "ひらけなかったよ" << std::endl;
            continue;
        }

        if (r.get_page_count() < 1) {
            continue;
        }

        const tiff::page& p = r.get_page(0);
        p.print_info();

        std::ofstream of(i + ".ppm");
        of << "P3" << std::endl;
        of << p.width << " " << p.height << std::endl;
        of << "255" << std::endl;
        for (int h = 0; h < p.height; h++) {
            for (int w = 0; w < p.width; w++) {
                auto c = p.get_pixel(w, h);
                of << +c.r << " " << +c.g << " " << +c.b << std::endl;
            }
        }
    }
}

