#include <cassert>
#include "gpu_core.h"
#include <memory/bus.h>
#include "vram.h"

gpu::state_t gpu::state;

uint32_t gpu::data() {
    if (gpu::state.gpu_to_cpu_transfer.run.active) {
        auto lower = vram_transfer();
        auto upper = vram_transfer();
        return (upper << 16) | lower;
    }

    return 0;
}

uint32_t gpu::stat() {
    return (gpu::state.status & ~0x00080000) | 0x1c002000;
}

uint32_t gpu::bus_read(int width, uint32_t address) {
    switch (address) {
    case 0x1f801810: return data();
    case 0x1f801814: return stat();
    }
}

void gpu::bus_write(int width, uint32_t address, uint32_t data) {
    switch (address) {
    case 0x1f801810:
        return gp0(data);

    case 0x1f801814:
        return gp1(data);
    }
}

uint16_t gpu::vram_transfer() {
    auto& transfer = gpu::state.gpu_to_cpu_transfer;
    if (!transfer.run.active) {
        return 0;
    }

    auto data = vram.read(transfer.reg.x + transfer.run.x,
        transfer.reg.y + transfer.run.y);

    transfer.run.x++;

    if (transfer.run.x == transfer.reg.w) {
        transfer.run.x = 0;
        transfer.run.y++;

        if (transfer.run.y == transfer.reg.h) {
            transfer.run.y = 0;
            transfer.run.active = false;
        }
    }

    return data;
}

void gpu::vram_transfer(uint16_t data) {
    auto& transfer = gpu::state.cpu_to_gpu_transfer;

    vram.write(transfer.reg.x + transfer.run.x,
        transfer.reg.y + transfer.run.y, data);

    transfer.run.x++;

    if (transfer.run.x == transfer.reg.w) {
        transfer.run.x = 0;
        transfer.run.y++;

        if (transfer.run.y == transfer.reg.h) {
            transfer.run.y = 0;
            transfer.run.active = false;
        }
    }
}
