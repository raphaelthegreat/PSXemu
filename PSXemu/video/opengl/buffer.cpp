#include "buffer.h"

Pos2 Pos2::from_gpu(uint32_t val)
{
    Pos2 pos;
    pos.x = (int16_t)bit_range(val, 0, 16);
    pos.y = (int16_t)bit_range(val, 16, 32);

    return pos;
}

Color Color::from_gpu(uint32_t val)
{
    Color color;
    color.red = (uint8_t)val;
    color.green = (uint8_t)(val >> 8);
    color.blue = (uint8_t)(val >> 16);

    return color;
}
