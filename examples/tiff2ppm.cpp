#include <bits/stdint-intn.h>
#include <iostream>
#include <fstream>
#include <string>
#include <type_traits>
#include <thread>

#include "tiff_reader.h"

int main(int argc, char** argv)
{
    std::vector<std::string> imgs;
    for (int i = 1; i < argc; i++) {
        imgs.emplace_back(argv[i]);
    }


    for (auto& i: imgs) {
        auto r = tiff::reader::open(i);
        if (!r.is_valid()) {
            std::cout << "Failed to open \"" << i << "\"" << std::endl;
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
        for (size_t h = 0; h < p.height; h++) {
            for (size_t w = 0; w < p.width; w++) {
                auto c = p.get_pixel(w, h);
                of << +c.r << " " << +c.g << " " << +c.b << std::endl;
            }
        }
    }
}

