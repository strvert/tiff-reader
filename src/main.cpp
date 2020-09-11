#include <bits/stdint-intn.h>
#include <iostream>
#include <fstream>
#include <string>
#include <type_traits>
#include "tiff_reader.h"

int main()
{
    std::vector<std::string> imgs;
    // imgs.push_back("./jp.tif");
    // imgs.push_back("./p_jp.tif");
    // imgs.push_back("./transparent.tiff");
    imgs.push_back("./be.tif");
    // imgs.push_back("./high_le.tiff");
    // imgs.push_back("./high_be.tiff");
    // imgs.push_back("./1MB.tiff");
    // imgs.push_back("./1MB_be.tiff");
    // imgs.push_back("./gray.tif");
    // imgs.push_back("./gray_be.tiff");

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

