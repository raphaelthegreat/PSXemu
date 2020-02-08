#include "interconnect.h"
#include <video/renderer.h>

std::optional<uint32_t> Range::contains(uint32_t addr) const
{
    if (addr >= start && addr < start + length)
        return (addr - start);
    else
        return std::nullopt;
}

Interconnect::Interconnect(std::string bios_path, Renderer* renderer) :
    dma(this)
{
    gl_renderer = renderer;
    
    bios = std::make_unique<Bios>(bios_path);
    ram = std::make_unique<Ram>();
    gpu = std::make_unique<GPU>(gl_renderer);
}

uint32_t Interconnect::mask_region(uint32_t addr)
{
    uint32_t index = addr >> 29;
    return (addr & region_mask[index]);
}