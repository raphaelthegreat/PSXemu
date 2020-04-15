#pragma once
#include "cdrom_disk.hpp"
#include <filesystem>
#include <initializer_list>
#include <deque>

/* IRQ delay in CD-ROM steps (each one is 100 CPU cycles). */
constexpr uint READ_SECTOR_DELAY_STEPS = 1150;
constexpr uint MAX_FIFO_SIZE = 16;

enum class CDResponse : ubyte {
    NoneInt0 = 0,     /* INT0: No response received (no interrupt request). */
    SecondInt1 = 1,   /* INT1: Received SECOND (or further) response to ReadS/ReadN (and Play+Report). */
    SecondInt2 = 2,   /* INT2: Received SECOND response (to various commands). */
    FirstInt3 = 3,    /* INT3: Received FIRST response (to any command). */
    DataEndInt4 = 4,  /* INT4: DataEnd (when Play/Forward reaches end of disk) (maybe also for Read?). */
    ErrorInt5 = 5,    /* INT5: Received error-code (in FIRST or SECOND response). */
};

inline uint operator &(CDResponse e, uint a)
{
    return a & (uint)e;
}

enum class CDReadState {
    Stopped,
    Seeking,
    Playing,
    Reading,
};

union CDSTAT {
    ubyte byte = 0;

    struct {
        ubyte index : 2;
        ubyte adpcm_fifo_empty : 1;         /* Set when playing XA-ADPCM sound. */
        ubyte param_fifo_empty : 1;         /* Triggered before writing 1st byte. */
        ubyte param_fifo_write_ready : 1;   /* Triggered after writing 16 bytes. */
        ubyte response_fifo_not_empty : 1;  /* Triggered after reading LAST byte. */
        ubyte data_fifo_not_empty : 1;      /* Triggered after reading LAST byte. */
        ubyte transmit_busy : 1;            /* Command/parameter transmission busy. */
    };
};

union CDMODE {
    ubyte byte = 0;

    struct {
        ubyte cd_da_read : 1;    // (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)
        ubyte auto_pause : 1;    // (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
        ubyte report : 1;        // (0=Off, 1=Enable Report-Interrupts for Audio Play)
        ubyte xa_filter : 1;     // (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
        ubyte ignore_bit : 1;    // (0=Normal, 1=Ignore Sector Size and Setloc position)
        ubyte sector_size : 1;  // (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
        ubyte xa_adpcm : 1;      // (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
        ubyte speed : 1;         // (0=Normal speed, 1=Double speed)
    };
};

union CDSTATCODE {
    ubyte byte = 0;

    struct {
        ubyte error : 1;
        ubyte spindle_motor_on : 1;
        ubyte seek_error : 1;
        ubyte id_error : 1;
        ubyte shell_open : 1;
        ubyte reading : 1;
        ubyte seeking : 1;
        ubyte playing : 1;
    };

    /* Does not reset shell open state. */
    void reset();
    void set_state(CDReadState state);
};

class Bus;
class CDManager {
public:
    CDManager(Bus* _bus);
    ~CDManager() = default;

    void insert_disk(const fs::path& file_path);
    void tick();
    
    /* Bus read/write */
    ubyte read(uint addr);
    void write(uint addr, ubyte data);
    
    ubyte read_byte();
    uint read_word();

private:
    void execute_command(ubyte cmd);
    inline void push_response(CDResponse type, std::initializer_list<ubyte> bytes);
    inline void push_response(CDResponse type, ubyte byte);
    inline void push_response_stat(CDResponse type);
    void command_error();
    ubyte get_param();
    bool is_data_buf_empty();

    static const char* get_reg_name(ubyte reg, ubyte index, bool is_read);
    static const char* get_cmd_name(ubyte cmd);

    inline uint sector_size() const;

private:
    CDDisk cd_disk;

    CDMODE mode;
    CDSTAT status;
    CDSTATCODE status_code;

    std::vector<ubyte> data_buffer, read_buffer;
    std::deque<ubyte> param_fifo, response_fifo;
    std::deque<CDResponse> irq_fifo;

    ubyte int_enable = 0;
    uint data_buffer_index = 0;
    uint seek_sector = 0, read_sector = 0;
    uint steps_until_read_sect = READ_SECTOR_DELAY_STEPS;

    bool muted = false;
    Bus* bus;
};
