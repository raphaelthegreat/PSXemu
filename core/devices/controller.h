#pragma once
#include <utility/types.hpp>

enum class Button {
    Select = 0x0,
    LAnalog = 0x1,
    RAnalog = 0x2,
    Start = 0x3,
    Up = 0x4,
    Right = 0x5,
    Down = 0x6,
    Left = 0x7,
    L2 = 0x8,
    R2 = 0x9,
    L1 = 0xA,
    R1 = 0xB,
    Trig = 0xC,
    Circle = 0xD,
    Cross = 0xE,
    Square = 0xF
};

enum class ControllerState {
    Idle,
    Transfer
};

enum class ControllerType {
    Digital = 0x5A41
};

/* Joypad status register. */
union JOYSTAT {
    uint value;

    struct {
        uint tx_ready_flag_1 : 1;
        uint rx_has_data : 1;
        uint tx_ready_flag_2 : 1;
        uint rx_parity_error : 1;
        uint : 3;
        uint ack_input_level : 1;
        uint : 1;
        uint irq_request : 1;
        uint : 1;
        uint baudrate_timer : 21;
    };
};

/* Joypad mode register. */
union JOYMODE {
    ushort value;

    struct {
        ushort reload_factor : 2;
        ushort char_length : 2;
        ushort parity_enable : 1;
        ushort parity_type : 1;
        ushort : 2;
        ushort clk_polarity : 1;
        ushort : 7;
    };
};

/* Joypad control register. */
union JOYCTRL {
    ushort value;

    struct {
        ushort tx_enable : 1;
        ushort joyn_output : 1;
        ushort rx_enable : 1;
        ushort : 1;
        uint acknowledge : 1;
        ushort : 1;
        ushort reset : 1;
        ushort : 1;
        ushort rx_irq_mode : 2;
        ushort tx_irq_enable : 1;
        ushort rx_irq_enable : 1;
        ushort ack_irq_enable : 1;
        ushort desired_slot : 1;
        ushort : 2;
    };
};

typedef ushort JOYBAUD;

class Controller {
public:
    Controller();
    virtual ~Controller() = default;

    void default_mapping();

    ubyte process(ubyte byte);
    void gen_response();
    ubyte dequeue();

    void key_down(int key);
    void key_up(int key);

public:
    ControllerState state = ControllerState::Idle;
    ControllerType type = ControllerType::Digital;

    std::deque<ubyte> transfer_fifo;
    std::unordered_map<int, Button> key_lookup;
    ushort buttons = 0xFFFF;
    bool enabled = true, ack = true;
};

class Bus;
class ControllerManager {
public:
    ControllerManager(Bus* _bus);
    ~ControllerManager() = default;

    bool tick();
    void reload();

    template <typename T>
    uint read(uint addr);

    template <typename T>
    void write(uint addr, uint value);

public:
    std::deque<ubyte> tx_data, rx_data;
    uint counter;

    JOYSTAT status;
    JOYMODE mode;
    JOYCTRL control;
    JOYBAUD baud;

    Controller controller;
    Bus* bus;
};

template<typename T>
inline uint ControllerManager::read(uint addr)
{
    uint offset = addr & 0xFF;
    if (offset == 0x40) {
        if (rx_data.empty()) return 0xFF;

        ubyte data = rx_data.front();
        rx_data.pop_front();
        return data;
    }
    else if (offset == 0x44) {
        status.ack_input_level = false;
        status.rx_has_data = !rx_data.empty();
        return status.value;
    }
    else if (offset == 0x48) {
        return mode.value;
    }
    else if (offset == 0x4A) {
        return control.value;
    }
    else if (offset == 0x4E) {
        return baud;
    }
    else {
        printf("[JOYPAD] Unhandled read at 0x%x\n", addr);
        return 0xFFFFFFFF;
    }    
}

template<typename T>
inline void ControllerManager::write(uint addr, uint value)
{
    uint offset = addr & 0xFF;    
    if (offset == 0x40) {
        tx_data.push_back((ubyte)value);        
        status.tx_ready_flag_1 = true;
        status.tx_ready_flag_2 = false;

        if (control.joyn_output) {
            status.tx_ready_flag_2 = true;

            if (control.desired_slot == 1) {
                tx_data.pop_front();
                rx_data.push_back(0xFF);
                return;
            }

            ubyte byte = tx_data.front();
            tx_data.pop_front();
            rx_data.push_back(controller.process(byte));

            control.acknowledge = controller.ack;
            status.ack_input_level = true;
        }

        if (control.acknowledge)
            counter = 500;
    }
    else if (offset == 0x48) {
        mode.value = value;
    }
    else if (offset == 0x4A) {
        control.value = value;

        if (control.acknowledge) {
            status.rx_parity_error = false;
            status.irq_request = false;
        }

        if (control.reset) {
            mode.value = 0;
            control.value = 0;
            baud = 0;

            rx_data.clear();
            tx_data.clear();

            status.tx_ready_flag_1 = true;
            status.tx_ready_flag_2 = true;
        }
    }
    else if (offset == 0x4E) {
        baud = (ushort)value;
        reload();
    }
    else {
        printf("[JOYPAD] Unhandled write at 0x%x\n", addr);
    }
}
