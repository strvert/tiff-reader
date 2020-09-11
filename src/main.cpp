#include <bits/stdint-intn.h>
#include <iostream>
#include <string>
#include <type_traits>
#include "tiff_reader.h"

int main()
{
    std::vector<std::string> imgs;
    // imgs.push_back("./jp.tif");
    // imgs.push_back("./p_jp.tif");
    imgs.push_back("./transparent.tiff");
    // imgs.push_back("./le.tif");
    // imgs.push_back("./be.tif");
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
            return 0;
        }

        std::cout << std::endl;
        r.print_header();
        std::cout << std::endl;
        r.decode().print_info();
        std::cout << std::endl;
    }
}

