#pragma once
#include <cstdint>
#include <array>
#include <bitset>
#include <vector>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using f32 = float;
using f64 = double;

using byte = u8;
using buffer = std::vector<byte>;
using address = u32;

union DmaChannelControl {
    u32 word{};

    struct {
        u32 transfer_direction : 1;
        u32 memory_address_step : 1;
        u32 _2_7 : 6;
        u32 chopping_enable : 1;
        u32 sync_mode : 2;
        u32 _11_15 : 5;
        u32 chopping_dma_window_size : 3;
        u32 _19 : 1;
        u32 chopping_cpu_window_size : 3;
        u32 _23 : 1;
        u32 enable : 1;
        u32 _25_27 : 3;
        u32 manual_trigger : 1;
        u32 _29_31 : 3;
    };
};

union DmaBlockControl {  // Fields are different depending on sync mode
    u32 word{};

    union {
        // Manual sync mode
        struct {
            u16 word_count;  // number of words to transfer
        } manual;
        // Request sync mode
        struct {
            u16 block_size;   // block size in words
            u16 block_count;  // number of blocks to transfer
        } request;
        // In Linked List mode this register is unused
    };
};

class DmaChannel {
public:
    enum class TransferDirection {
        ToRam = 0,
        FromRam = 1,
    };
    enum class MemoryAddressStep {
        Forward = 0,
        Backward = 1,
    };
    enum class SyncMode {
        // Transfer starts when CPU writes to the manual_trigger bit and happens all at once
        // Used for CDROM, OTC
        Manual = 0,
        // Sync blocks to DMA requests
        // Used for MDEC, SPU, and GPU-data
        Request = 1,
        // Used for GPU-command-lists
        LinkedList = 2,
    };

    bool active() const;
    u32 transfer_word_count();
    void transfer_finished();
    TransferDirection transfer_direction() const {
        return static_cast<TransferDirection>(m_channel_control.transfer_direction);
    }
    bool to_ram() const { return transfer_direction() == TransferDirection::ToRam; }
    MemoryAddressStep memory_address_step() const {
        return static_cast<MemoryAddressStep>(m_channel_control.memory_address_step);
    }
    SyncMode sync_mode() const { return static_cast<SyncMode>(m_channel_control.sync_mode); }
    const char* sync_mode_str() const {
        switch (sync_mode()) {
        case SyncMode::Manual: return "Manual";
        case SyncMode::Request: return "Request";
        case SyncMode::LinkedList: return "Linked List";
        default: return "<Invalid>";
        }
    }

    DmaChannelControl m_channel_control;
    DmaBlockControl m_block_control;
    u32 m_base_addr{};
};

enum class DmaPort {
    MdecIn = 0,   // Macroblock decoder input
    MdecOut = 1,  // Macroblock decoder output
    Gpu = 2,      // Graphics Processing Unit
    Cdrom = 3,    // CD-ROM drive
    Spu = 4,      // Sound Processing Unit
    Pio = 5,      // Extension port
    Otc = 6,      // Used to clear the ordering table
};

static const char* dma_port_to_str(DmaPort dma_port) {
    switch (dma_port) {
    case DmaPort::MdecIn: return "MDECin";
    case DmaPort::MdecOut: return "MDECout";
    case DmaPort::Gpu: return "GPU";
    case DmaPort::Cdrom: return "CD-ROM";
    case DmaPort::Spu: return "SPU";
    case DmaPort::Pio: return "PIO";
    case DmaPort::Otc: return "OTC";
    default: return "<Invalid>";
    }
}

union DmaInterruptRegister {
    u32 word{};

    struct {
        u32 _0_14 : 15;
        u32 force : 1;

        u32 dec_in_enable : 1;
        u32 dec_out_enable : 1;
        u32 gpu_enable : 1;
        u32 cdrom_enable : 1;
        u32 spu_enable : 1;
        u32 ext_enable : 1;
        u32 ram_enable : 1;
        u32 master_enable : 1;

        u32 dec_in_flags : 1;
        u32 dec_out_flags : 1;
        u32 gpu_flags : 1;
        u32 cdrom_flags : 1;
        u32 spu_flags : 1;
        u32 ext_flags : 1;
        u32 ram_flags : 1;
        u32 master_flags : 1;
    };

    bool is_port_enabled(DmaPort port) const { return (word & (1 << (16 + (u8)port))) || master_enable; }
    void set_port_flags(DmaPort port, bool val) {
        std::bitset<32> bs(word);
        bs.set((u8)port + 24, val);
        word = bs.to_ulong();
    }

    bool get_irq_master_flag() {
        u8 all_enable = (word & 0x7F0000) >> 16;
        u8 all_flag = (word & 0x7F000000) >> 24;
        return force || (master_enable && (all_enable & all_flag));
    }
};

class Bus;
class DMAController {
public:
    DMAController(Bus* _bus)
        : bus(_bus) {}

    template <typename ValueType>
    ValueType read(address addr_rebased) const {
        const auto major = (addr_rebased & 0x70) >> 4;
        const auto minor = addr_rebased & 0b1100;

        const u32* reg = nullptr;

        if (0 <= major && major <= 6) {
            // Per-channel registers
            const auto& channel = channel_control(static_cast<DmaPort>(major));

            switch (minor) {
            case 0: reg = &channel.m_base_addr; break;
            case 4: reg = &channel.m_block_control.word; break;
            case 8: reg = &channel.m_channel_control.word; break;
            //default: LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", addr_rebased); return 0;
            }
        }
        else if (major == 7) {
            // Common registers
            switch (minor) {
            case 0: reg = &m_reg_control; break;
            case 4: reg = &m_reg_interrupt.word; break;
            //default: LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", addr_rebased); break;
            }
        }
        else {
            //LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", addr_rebased);
            return 0;
        }

        // Do the read
        const u32 reg_offset = addr_rebased & 0b11;
        return *(ValueType*)((u8*)reg + reg_offset);
    }

    template <typename ValueType>
    void write(address addr_rebased, ValueType val) {
        const auto major = (addr_rebased & 0x70) >> 4;
        const auto minor = addr_rebased & 0b1100;

        const u32* reg = nullptr;
        DmaChannel* channel = nullptr;

        if (0 <= major && major <= 6) {
            // Per-channel registers
            channel = &channel_control(static_cast<DmaPort>(major));

            switch (minor) {
            case 0: reg = &channel->m_base_addr; break;
            case 4: reg = &channel->m_block_control.word; break;
            case 8: reg = &channel->m_channel_control.word; break;
            default:
                //LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, addr_rebased);
                return;
            }
        }
        else if (major == 7) {
            // Common registers
            switch (minor) {
            case 0: reg = &m_reg_control; break;
            case 4: {
                // Handle u32 case separately because we need to do masking

                if (std::is_same<ValueType, u8>::value || std::is_same<ValueType, u16>::value) {
                    reg = &m_reg_interrupt.word;
                    break;
                }
                else if (std::is_same<ValueType, u32>::value) {
                    // Clear acknowledged (1'ed) flag bits
                    u32 masked_flags = (((m_reg_interrupt.word & 0xFF000000) & ~(val & 0xFF000000)));
                    m_reg_interrupt.word = (val & 0x00FFFFFF) | masked_flags;
                    return;  // write handled here, we can return instead of breaking
                }
                else
                    static_assert("16-bit write unsuported");
            }
            default:
                //LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, addr_rebased);
                return;
            }
        }
        else {
            //LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, addr_rebased);
            return;
        }

        // Do the write
        const u32 reg_offset = addr_rebased & 0b11;
        *(ValueType*)((u8*)reg + reg_offset) = val;

        // Handle activated transfers
        if (channel && channel->active()) {
            const auto port = static_cast<DmaPort>(major);
            do_transfer(port);
        }
    }

    DmaChannel const& channel_control(DmaPort port) const;
    DmaChannel& channel_control(DmaPort port);
    void tick();

private:
    void do_transfer(DmaPort port);
    void do_block_transfer(DmaPort port);
    void transfer_finished(DmaChannel& channel, DmaPort port);
    void do_linked_list_transfer(DmaPort port);

private:
    u32 m_reg_control{ 0x07654321 };
    DmaInterruptRegister m_reg_interrupt;
    bool m_irq_pending{};

    std::array<DmaChannel, 7> m_channels{};

    Bus* bus;
};