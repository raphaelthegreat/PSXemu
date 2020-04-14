#include <stdafx.hpp>
#include "bios.h"

Bios::Bios(std::string path)
{
    FILE* in = fopen(path.c_str(), "rb");
    fread(bios, 1, 512 * 1024, in);
    fclose(in);
}