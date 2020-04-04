#include <stdafx.hpp>

#include "cdrom_drive.hpp"
#include <memory/bus.h>

void CDSTATCODE::reset()
{
    error = 0;
    spindle_motor_on = 0;
    seek_error = 0;
    id_error = 0;
    reading = 0;
    seeking = 0;
    playing = 0;
}

void CDSTATCODE::set_state(CDReadState state)
{
    reset();
    spindle_motor_on = true;  // Turn on motor
    switch (state) {
    case CDReadState::Seeking: seeking = true; break;
    case CDReadState::Playing: playing = true; break;
    case CDReadState::Reading: reading = true; break;
    }
}

CDManager::CDManager(Bus* _bus)
{
    status.param_fifo_empty = true;
    status.param_fifo_write_ready = true;

    status_code.shell_open = true;

    bus = _bus;
}

void CDManager::insert_disk(const fs::path& file_path) {
    auto extension = file_path.extension().string();
    
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".bin")
        cd_disk.parse_bin(file_path.string().c_str());
    else
        printf("[CDROM] Unhandled disk format!\n");

    status_code.shell_open = false;
}

void CDManager::tick() {
    status.transmit_busy = false;

    if (!irq_fifo.empty()) {
        auto irq_triggered = irq_fifo.front() & 0b111;
        auto irq_mask = int_enable & 0b111;

        if (irq_triggered & irq_mask)
            bus->irq(Interrupt::CDROM);
    }

    constexpr std::array<ubyte, 12> SYNC_MAGIC = { { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                  0xff, 0xff, 0x00 } };

    if (status_code.reading || status_code.playing) {
        if (--steps_until_read_sect == 0) {
            steps_until_read_sect = READ_SECTOR_DELAY_STEPS;

            DataType sector_type;
            const auto pos_to_read = CDPos::from_lba(read_sector);
            read_buffer = cd_disk.read(pos_to_read, sector_type);

            read_sector++;

            if (sector_type == DataType::Invalid)
                return;

            const auto sector_has_data = (sector_type == DataType::Data);
            const auto sector_has_audio = (sector_type == DataType::Audio);

            auto sync_match = std::equal(SYNC_MAGIC.begin(), SYNC_MAGIC.end(), read_buffer.begin());

            if (status_code.playing && sector_has_audio) {  // Reading audio
                if (sync_match)
                    printf("Sync data found in Audio sector\n");
            }
            else if (status_code.reading && sector_has_data) {  // Reading data
                if (!sync_match)
                    printf("Sync data mismach in Data sector\n");

                // ack more data
                push_response(CDResponse::SecondInt1, status_code.byte);
            }
        }
    }
}

ubyte CDManager::read(uint addr_rebased) {
    const ubyte reg = addr_rebased;
    const ubyte reg_index = status.index;

    ubyte val = 0;

    if (reg == 0) {  // Status Register
        val = status.byte;
    }
    else if (reg == 1) {  // Response FIFO
        if (!response_fifo.empty()) {
            val = response_fifo.front();
            response_fifo.pop_front();

            if (response_fifo.empty())
                status.response_fifo_not_empty = false;
        }
    }
    else if (reg == 2) {  // Data FIFO
        val = read_byte();
    }
    else if (reg == 3 && (reg_index == 0 || reg_index == 2)) {  // Interrupt Enable Register
        val = int_enable;
    }
    else if (reg == 3 && (reg_index == 1 || reg_index == 3)) {  // Interrupt Flag Register
        val = 0b11100000;                                           // these bits are always set

        if (!irq_fifo.empty())
            val |= irq_fifo.front() & 0b111;
    }
    else {
        //LOG_ERROR_CDROM("Unknown combination, CDREG{}.{}", reg, reg_index);
    }

    //LOG_TRACE_CDROM("CDROM read {} (CDREG{}.{}) val: 0x{:02X} ({:#010b})",
        //get_reg_name(reg, reg_index, true), reg, reg_index, val, val);

    return val;
}

bool CDManager::is_data_buf_empty() {
    if (data_buffer.empty())
        return true;

    const auto sector_size = this->sector_size();
    if (data_buffer_index >= sector_size)
        return true;

    return false;
}

void CDManager::write(uint addr_rebased, ubyte val) {
    const ubyte reg = addr_rebased;
    const ubyte reg_index = status.index;

    if (reg == 0) {  // Index Register
        status.index = val & 0b11;
        return;                                 // Don't log in this case
    }
    else if (reg == 1 && reg_index == 0) {  // Command Register
        execute_command(val);
    }
    else if (reg == 1 && reg_index == 1) {  // Sound Map Data Out
    }
    else if (reg == 1 && reg_index == 2) {  // Sound Map Coding Info
    }
    else if (reg == 1 && reg_index == 3) {  // Audio Volume for Right-CD-Out to Right-SPU-Input
    }
    else if (reg == 2 && reg_index == 0) {  // Parameter FIFO
        assert(param_fifo.size() < MAX_FIFO_SIZE);

        param_fifo.push_back(val);
        status.param_fifo_empty = false;
        status.param_fifo_write_ready = (param_fifo.size() < MAX_FIFO_SIZE);
    }
    else if (reg == 2 && reg_index == 1) {  // Interrupt Enable Register
        int_enable = val;
    }
    else if (reg == 2 && reg_index == 2) {  // Audio Volume for Left-CD-Out to Left-SPU-Input
    }
    else if (reg == 2 && reg_index == 3) {  // Audio Volume for Right-CD-Out to Left-SPU-Input
    }
    else if (reg == 3 && reg_index == 0) {  // Request Register
        if (val & 0x80) {                       // Want data
            if (is_data_buf_empty()) {  // Only update data buffer if everything from it has been read
                data_buffer = std::move(read_buffer);
                data_buffer_index = 0;
                status.data_fifo_not_empty = true;
            }
        }
        else {  // Clear data buffer
            data_buffer.clear();
            data_buffer_index = 0;
            status.data_fifo_not_empty = false;
        }
    }
    else if (reg == 3 && reg_index == 1) {  // Interrupt Flag Register
        if (val & 0x40) {                       // Reset Parameter FIFO
            param_fifo.clear();
            status.param_fifo_empty = true;
            status.param_fifo_write_ready = true;
        }
        if (!irq_fifo.empty())
            irq_fifo.pop_front();
    }
    else if (reg == 3 && reg_index == 2) {  // Audio Volume for Left-CD-Out to Right-SPU-Input
    }
    else if (reg == 3 && reg_index == 3) {  // Audio Volume Apply Changes
    }
    else {
        //LOG_ERROR_CDROM("Unknown combination, CDREG{}.{} val: {:02X}", reg, reg_index, val);
    }

    //LOG_TRACE_CDROM("CDROM write {} (CDREG{}.{}) val: 0x{:02X} ({:#010b})",
    //    get_reg_name(reg, reg_index, false), reg, reg_index, val, val);
}

ubyte CDManager::read_byte() {
    if (is_data_buf_empty()) {
        printf("Tried to read with an empty buffer\n");
        return 0;
    }

    // TODO reads out of bounds
    const bool data_only = (sector_size() == 0x800);

    uint data_offset = data_only ? 24 : 12;

    ubyte data = data_buffer[data_offset + data_buffer_index];
    ++data_buffer_index;

    if (is_data_buf_empty())
        status.data_fifo_not_empty = false;

    return data;
}

uint CDManager::read_word() {
    uint data{};
    data |= read_byte() << 0;
    data |= read_byte() << 8;
    data |= read_byte() << 16;
    data |= read_byte() << 24;
    return data;
}

void CDManager::execute_command(ubyte cmd) {
    irq_fifo.clear();
    response_fifo.clear();

    //LOG_DEBUG_CDROM("CDROM command issued: {} ({:02X})", get_cmd_name(cmd), cmd);

    //if (!param_fifo.empty())
    //    LOG_DEBUG_CDROM("Parameters: [{:02X}]", fmt::join(param_fifo, ", "));

    switch (cmd) {
    case 0x01:  // Getstat
        push_response_stat(CDResponse::FirstInt3);
        break;
    case 0x02: {  // Setloc
        const auto mm = util::bcd_to_dec(get_param());
        const auto ss = util::bcd_to_dec(get_param());
        const auto ff = util::bcd_to_dec(get_param());

        CDPos pos(mm, ss, ff);

        seek_sector = pos.to_lba();

        push_response_stat(CDResponse::FirstInt3);
        break;
    }
    case 0x03:                        // Play
        assert(param_fifo.empty());  // we don't handle the parameter
        read_sector = seek_sector;

        status_code.set_state(CDReadState::Playing);

        push_response_stat(CDResponse::FirstInt3);
        break;
    case 0x06:  // ReadN
        read_sector = seek_sector;

        status_code.set_state(CDReadState::Reading);

        push_response_stat(CDResponse::FirstInt3);
        break;
    case 0x07:  // MotorOn
        status_code.spindle_motor_on = true;

        push_response_stat(CDResponse::FirstInt3);
        push_response_stat(CDResponse::SecondInt2);
        break;
    case 0x08:  // Stop
        status_code.set_state(CDReadState::Stopped);
        status_code.spindle_motor_on = false;

        push_response_stat(CDResponse::FirstInt3);
        push_response_stat(CDResponse::SecondInt2);
        break;
    case 0x09:  // Pause
        push_response_stat(CDResponse::FirstInt3);

        status_code.set_state(CDReadState::Stopped);

        push_response_stat(CDResponse::SecondInt2);
        break;
    case 0x0D: {
        param_fifo.clear();
        push_response_stat(CDResponse::FirstInt3);
        break;
    }
    case 0x0E: {  // Setmode
        push_response_stat(CDResponse::FirstInt3);

        // TODO: bit 4 behaviour
        const auto param = get_param();
        assert(!(param & 0b10000));
        mode.byte = param;
        break;
    }
    case 0x0B:  // Mute
        muted = true;

        push_response_stat(CDResponse::FirstInt3);
        break;
    case 0x0C:  // Demute
        muted = false;

        push_response_stat(CDResponse::FirstInt3);
        break;
    case 0x0F:                                                     // Getparam
        push_response(CDResponse::FirstInt3, { status_code.byte, 0x00, 0x00 });  // TODO: last arg is the filter
        break;
    case 0x11: {
        push_response(CDResponse::FirstInt3, { 0, 0, 0, 0, 0, 0, 0, 0 });
        break;
    }
    case 0x13: {                                  // GetTN
        const auto index = util::dec_to_bcd(0x01);  // TODO
        const auto track_count = util::dec_to_bcd(cd_disk.get_track_count());
        push_response(CDResponse::FirstInt3, { status_code.byte, index, track_count });
        break;
    }
    case 0x14: {  // GetTD
        const auto track_number = util::bcd_to_dec(get_param());

        CDPos disk_pos{};
        if (track_number == 0) {  // Special meaning: last track (total size)
            disk_pos = cd_disk.size();
        }
        else {  // Start of a track
            disk_pos = cd_disk.get_track_start(track_number);
        }

        ubyte minutes = util::dec_to_bcd(disk_pos.minutes);
        ubyte seconds = util::dec_to_bcd(disk_pos.seconds);

        push_response(CDResponse::FirstInt3, { status_code.byte, minutes, seconds });
        break;
    }
    case 0x15: {  // SeekL
        push_response_stat(CDResponse::FirstInt3);

        read_sector = seek_sector;
        status_code.set_state(CDReadState::Seeking);

        push_response_stat(CDResponse::SecondInt2);
        break;
    }
    case 0x19: {  // Test
        const auto subfunction = get_param();

        //LOG_DEBUG_CDROM("  CDROM command subfuncion: {:02X}", subfunction);

        switch (subfunction) {
        case 0x20:  // Get CDROM BIOS date/version (yy,mm,dd,ver)
            push_response(
                CDResponse::FirstInt3,
                { 0x94, 0x09, 0x19, 0xC0 });  // Send reponse of PSX (PU-7), 18 Nov 1994, version vC0 (b)
            break;
        default: {
            command_error();
            //LOG_ERROR_CDROM("Unhandled Test subfunction {:02X}", subfunction);
            break;
        }
        }
        break;
    }
    case 0x1A: {  // GetID
        const bool has_disk = !cd_disk.is_empty();

        if (status_code.shell_open) {
            push_response(CDResponse::ErrorInt5, { 0x11, 0x80 });
        }
        else if (has_disk) {  // Disk
            push_response(CDResponse::FirstInt3, status_code.byte);
            push_response(CDResponse::SecondInt2, { 0x02, 0x00, 0x20, 0x00, 'S', 'C', 'E', 'A' });
        }
        else {  // No Disk
            push_response(CDResponse::FirstInt3, status_code.byte);
            push_response(CDResponse::ErrorInt5, { 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        }
        break;
    }
    case 0x1B:  // ReadS
        read_sector = seek_sector;

        status_code.set_state(CDReadState::Reading);

        push_response_stat(CDResponse::FirstInt3);
        break;
    case 0x0A: {  // Init
        push_response_stat(CDResponse::FirstInt3);

        status_code.reset();
        status_code.spindle_motor_on = true;

        mode.byte = 0;

        push_response_stat(CDResponse::SecondInt2);
        break;
    }
    default: {
        command_error();
        printf("Unhandled CDROM command 0x%x", cmd);
        exit(0);
        break;
    }
    }

    param_fifo.clear();

    status.transmit_busy = true;
    status.param_fifo_empty = true;
    status.param_fifo_write_ready = true;
    status.adpcm_fifo_empty = false;
}

void CDManager::command_error() {
    push_response(CDResponse::ErrorInt5, { 0x11, 0x40 });
}

ubyte CDManager::get_param() {
    assert(!param_fifo.empty());

    auto param = param_fifo.front();
    param_fifo.pop_front();

    status.param_fifo_empty = param_fifo.empty();
    status.param_fifo_write_ready = true;

    return param;
}

inline void CDManager::push_response(CDResponse type, std::initializer_list<ubyte> bytes) {
    // First we write the type (INT value) in the Interrupt FIFO
    irq_fifo.push_back(type);

    // Then we write the response's data (args) to the Response FIFO
    for (auto response_byte : bytes) {
        if (response_fifo.size() < MAX_FIFO_SIZE) {
            response_fifo.push_back(response_byte);
            status.response_fifo_not_empty = true;
        }
    }
}

inline void CDManager::push_response(CDResponse type, ubyte byte) {
    push_response(type, { byte });
}

inline void CDManager::push_response_stat(CDResponse type) {
    push_response(type, status_code.byte);
}

const char* CDManager::get_cmd_name(ubyte cmd) {
    const char* cmd_names[] = { "Sync",       "Getstat",   "Setloc",  "Play",     "Forward", "Backward",
                                "ReadN",      "MotorOn",   "Stop",    "Pause",    "Init",    "Mute",
                                "Demute",     "Setfilter", "Setmode", "Getparam", "GetlocL", "GetlocP",
                                "SetSession", "GetTN",     "GetTD",   "SeekL",    "SeekP",   "-",
                                "-",          "Test",      "GetID",   "ReadS",    "Reset",   "GetQ",
                                "ReadTOC",    "VideoCD" };

    if (cmd <= 0x1F)
        return cmd_names[cmd];
    if (0x50 <= cmd && cmd <= 0x57)
        return "Secret";
    return "<unknown>";
}

inline uint CDManager::sector_size() const
{
    return mode.sector_size ? 0x924 : 0x800;
}

const char* CDManager::get_reg_name(ubyte reg, ubyte index, bool is_read) {
    // clang-format off
    if (is_read) {
        if (reg == 0) return "Status Register";
        if (reg == 1 && index == 0) return "Command Register";
        if (reg == 1) return "Response FIFO";
        if (reg == 2) return "Data FIFO";
        if (reg == 3 && (index == 0 || index == 2)) return "Interrupt Enable Register";
        if (reg == 3 && (index == 1 || index == 3)) return "Interrupt Flag Register";
    }
    else {
        if (reg == 0) return "Index Register";
        if (reg == 1 && index == 0) return "Command Register";
        if (reg == 1 && index == 1) return "Sound Map Data Out";
        if (reg == 1 && index == 2) return "Sound Map Coding Info";
        if (reg == 1 && index == 3) return "Audio Volume for Right-CD-Out to Right-SPU-Input";
        if (reg == 2 && index == 0) return "Parameter FIFO";
        if (reg == 2 && index == 1) return "Interrupt Enable Register";
        if (reg == 2 && index == 2) return "Audio Volume for Left-CD-Out to Left-SPU-Input";
        if (reg == 2 && index == 3) return "Audio Volume for Right-CD-Out to Left-SPU-Input";
        if (reg == 3 && index == 0) return "Request Register";
        if (reg == 3 && index == 1) return "Interrupt Flag Register";
        if (reg == 3 && index == 2) return "Audio Volume for Left-CD-Out to Right-SPU-Input";
        if (reg == 3 && index == 3) return "Audio Volume Apply Changes";
    }
    // clang-format on
    return "<unknown>";
}
