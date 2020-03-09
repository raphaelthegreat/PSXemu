#pragma once

#include "cdrom_disk.hpp"
#include <filesystem>
#include <deque>
#include <initializer_list>

class Bus;

namespace io {

constexpr auto READ_SECTOR_DELAY_STEPS = 1150;  // IRQ delay in CD-ROM steps (each one is 100 CPU cycles)
constexpr size_t MAX_FIFO_SIZE = 16;

enum CdromResponseType : uint8_t {
  NoneInt0 = 0,     // INT0: No response received (no interrupt request)
  SecondInt1 = 1,   // INT1: Received SECOND (or further) response to ReadS/ReadN (and Play+Report)
  SecondInt2 = 2,   // INT2: Received SECOND response (to various commands)
  FirstInt3 = 3,    // INT3: Received FIRST response (to any command)
  DataEndInt4 = 4,  // INT4: DataEnd (when Play/Forward reaches end of disk) (maybe also for Read?)
  ErrorInt5 = 5,    // INT5: Received error-code (in FIRST or SECOND response)
};

enum class CdromReadState {
  Stopped,
  Seeking,
  Playing,
  Reading,
};

union CdromStatusRegister {
  uint8_t byte{};

  struct {
    uint8_t index : 2;
    uint8_t adpcm_fifo_empty : 1;         // set when playing XA-ADPCM sound
    uint8_t param_fifo_empty : 1;         // triggered before writing 1st byte
    uint8_t param_fifo_write_ready : 1;   // triggered after writing 16 bytes
    uint8_t response_fifo_not_empty : 1;  // triggered after reading LAST byte
    uint8_t data_fifo_not_empty : 1;      // triggered after reading LAST byte
    uint8_t transmit_busy : 1;            // Command/parameter transmission busy
  };

  CdromStatusRegister() {
    param_fifo_empty = true;
    param_fifo_write_ready = true;
  }
};

union CdromMode {
  uint8_t byte{};

  struct {
    uint8_t cd_da_read : 1;    // (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)
    uint8_t auto_pause : 1;    // (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
    uint8_t report : 1;        // (0=Off, 1=Enable Report-Interrupts for Audio Play)
    uint8_t xa_filter : 1;     // (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
    uint8_t ignore_bit : 1;    // (0=Normal, 1=Ignore Sector Size and Setloc position)
    uint8_t _sector_size : 1;  // (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
    uint8_t xa_adpcm : 1;      // (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
    uint8_t speed : 1;         // (0=Normal speed, 1=Double speed)
  };

  void reset() { byte = 0; }
  uint32_t sector_size() const { return _sector_size ? 0x924 : 0x800; }
};

union CdromStatusCode {
  uint8_t byte{};

  struct {
    uint8_t error : 1;
    uint8_t spindle_motor_on : 1;
    uint8_t seek_error : 1;
    uint8_t id_error : 1;
    uint8_t shell_open : 1;
    uint8_t reading : 1;
    uint8_t seeking : 1;
    uint8_t playing : 1;
  };

  CdromStatusCode() { shell_open = true; }

  // Does not reset shell open state
  void reset() { error = spindle_motor_on = seek_error = id_error = reading = seeking = playing = 0; }

  void set_state(CdromReadState state) {
    reset();
    spindle_motor_on = true;  // Turn on motor
    switch (state) {
      case CdromReadState::Seeking: seeking = true; break;
      case CdromReadState::Playing: playing = true; break;
      case CdromReadState::Reading: reading = true; break;
    }
  }
};

class CdromDrive {
 public:
  void init(Bus* _bus);
  void insert_disk_file(const fs::path& file_path);
  void step();
  uint8_t read_reg(uint32_t addr_rebased);
  void write_reg(uint32_t addr_rebased, uint8_t val);
  uint8_t read_byte();
  uint32_t read_word();

 private:
  void execute_command(uint8_t cmd);
  void push_response(CdromResponseType type, std::initializer_list<uint8_t> bytes);
  void push_response(CdromResponseType type, uint8_t byte);
  void push_response_stat(CdromResponseType type);
  void command_error();
  uint8_t get_param();
  bool is_data_buf_empty();

  static const char* get_reg_name(uint8_t reg, uint8_t index, bool is_read);
  static const char* get_cmd_name(uint8_t cmd);

 private:
  CdromDisk m_disk;

  CdromStatusRegister m_reg_status{};
  CdromStatusCode m_stat_code{};
  CdromMode m_mode{};

  uint32_t m_seek_sector{};
  uint32_t m_read_sector{};

  std::deque<uint8_t> m_param_fifo{};
  std::deque<CdromResponseType> m_irq_fifo{};
  std::deque<uint8_t> m_resp_fifo{};

  uint8_t m_reg_int_enable{};
  uint32_t m_steps_until_read_sect{ READ_SECTOR_DELAY_STEPS };

  buffer m_read_buf{};
  buffer m_data_buf{};
  uint32_t m_data_buffer_index{};

  bool m_muted{ false };

  Bus* bus;
};

}  // namespace io
