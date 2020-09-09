#include <bits/stdint-intn.h>
#include <iostream>
#include <string>
#include <type_traits>
#include "tiff_reader.h"

int main()
{
    std::string imgs[] = {"./jp.tif", "./transparent.tiff"};
    for (auto& i: imgs) {
        auto r = tiff::reader::open(i);
        if (!r.is_valid()) {
            std::cout << "ひらけなかったよ" << std::endl;
            return 0;
        }

        r.print_header();

        r.decode().print_info();
    }
}

