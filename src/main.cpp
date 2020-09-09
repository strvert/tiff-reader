#include <bits/stdint-intn.h>
#include <iostream>
#include <string>
#include <type_traits>
#include "tiff_reader.h"

int main()
{
    auto r = tiff::reader::open("./jp.tif");
    if (!r.is_valid()) {
        std::cout << "ひらけなかったよ" << std::endl;
        return 0;
    }

    r.print_header();
}

