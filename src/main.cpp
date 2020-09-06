#include <iostream>
#include <string>
#include <type_traits>
#include "tiff_reader.h"

int func(int a)
{
    return a;
}

template<typename T>
T func_t();

template<>
int func_t()
{
    return 10;
}

template<>
float func_t()
{
    return 12.3f;
}

int main()
{
    auto r = tiff_reader::open("./jp.tif");
    if (!r.is_valid()) {
        std::cout << "ひらけなかったよ" << std::endl;
        return 0;
    }

    r.print_header();
}

