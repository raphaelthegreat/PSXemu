#pragma once
#include <cstdint>
#include <iostream>
#include <deque>

//#pragma optimize("", off)

#define GLFW_KEY_SPACE              32
#define GLFW_KEY_APOSTROPHE         39  /* ' */
#define GLFW_KEY_COMMA              44  /* , */
#define GLFW_KEY_MINUS              45  /* - */
#define GLFW_KEY_PERIOD             46  /* . */
#define GLFW_KEY_SLASH              47  /* / */
#define GLFW_KEY_0                  48
#define GLFW_KEY_1                  49
#define GLFW_KEY_2                  50
#define GLFW_KEY_3                  51
#define GLFW_KEY_4                  52
#define GLFW_KEY_5                  53
#define GLFW_KEY_6                  54
#define GLFW_KEY_7                  55
#define GLFW_KEY_8                  56
#define GLFW_KEY_9                  57
#define GLFW_KEY_SEMICOLON          59  /* ; */
#define GLFW_KEY_EQUAL              61  /* = */
#define GLFW_KEY_A                  65
#define GLFW_KEY_B                  66
#define GLFW_KEY_C                  67
#define GLFW_KEY_D                  68
#define GLFW_KEY_E                  69
#define GLFW_KEY_F                  70
#define GLFW_KEY_G                  71
#define GLFW_KEY_H                  72
#define GLFW_KEY_I                  73
#define GLFW_KEY_J                  74
#define GLFW_KEY_K                  75
#define GLFW_KEY_L                  76
#define GLFW_KEY_M                  77
#define GLFW_KEY_N                  78
#define GLFW_KEY_O                  79
#define GLFW_KEY_P                  80
#define GLFW_KEY_Q                  81
#define GLFW_KEY_R                  82
#define GLFW_KEY_S                  83
#define GLFW_KEY_T                  84
#define GLFW_KEY_U                  85
#define GLFW_KEY_V                  86
#define GLFW_KEY_W                  87
#define GLFW_KEY_X                  88
#define GLFW_KEY_Y                  89
#define GLFW_KEY_Z                  90
#define GLFW_KEY_LEFT_BRACKET       91  /* [ */
#define GLFW_KEY_BACKSLASH          92  /* \ */
#define GLFW_KEY_RIGHT_BRACKET      93  /* ] */
#define GLFW_KEY_GRAVE_ACCENT       96  /* ` */
#define GLFW_KEY_WORLD_1            161 /* non-US #1 */
#define GLFW_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define GLFW_KEY_ESCAPE             256
#define GLFW_KEY_ENTER              257
#define GLFW_KEY_TAB                258
#define GLFW_KEY_BACKSPACE          259
#define GLFW_KEY_INSERT             260
#define GLFW_KEY_DELETE             261
#define GLFW_KEY_RIGHT              262
#define GLFW_KEY_LEFT               263
#define GLFW_KEY_DOWN               264
#define GLFW_KEY_UP                 265
#define GLFW_KEY_PAGE_UP            266
#define GLFW_KEY_PAGE_DOWN          267
#define GLFW_KEY_HOME               268
#define GLFW_KEY_END                269
#define GLFW_KEY_CAPS_LOCK          280
#define GLFW_KEY_SCROLL_LOCK        281
#define GLFW_KEY_NUM_LOCK           282
#define GLFW_KEY_PRINT_SCREEN       283
#define GLFW_KEY_PAUSE              284

class Controller {
public:
    uint16_t CONTROLLER_TYPE = 0x5A41; //digital

    enum Mode {
        Idle,
        Transfer
    };

    Mode mode = Mode::Idle;

    std::deque<uint8_t> transferDataFifo;
    uint16_t buttons = 0xFFFF;
    bool ack;

    uint8_t process(uint8_t b) {
        switch (mode) {
        case Mode::Idle: {
            switch (b) {
            case 0x01: {
                //Console.WriteLine("[Controller] Idle Process 0x01");
                mode = Mode::Transfer;
                enabled = true;
                ack = true;
                return 0xFF;
            }
            default: {
                //Console.WriteLine("[Controller] Idle value WARNING " + b);
                ack = false;
                return 0xFF;
            }
            }
        }
        case Mode::Transfer: {
            switch (b) {
            case 0x01: {
                //Console.WriteLine("[Controller] ERROR Transfer Process 0x1");
                ack = true;
                return 0xFF;
            }
            case 0x42: {
                //Console.WriteLine("[Controller] Transfer Process 0x42");
                mode = Mode::Transfer;
                ack = true;
                generateResponse();
                uint8_t data = transferDataFifo.front();
                transferDataFifo.pop_front();
                return data;
            }
            default: {
                //Console.WriteLine("[Controller] Transfer Process" + b.ToString("x2"));
                uint8_t data;
                if (transferDataFifo.empty()) {
                    //Console.WriteLine("Changing to mode IDLE");
                    enabled = false;
                    mode = Mode::Idle;
                    ack = false;
                    data = 0xFF;
                }
                else {
                    data = transferDataFifo.front();
                    transferDataFifo.pop_front();
                }
                return data;
            }
            }
        }
        default: {
            //Console.WriteLine("[JOYPAD] Mode Warning");
            ack = false;
            return 0xFF;
        }
        }
    }

    bool enabled;

    void generateResponse() {
        uint8_t b0 = (uint8_t)(CONTROLLER_TYPE & 0xFF);
        uint8_t b1 = (uint8_t)((CONTROLLER_TYPE >> 8) & 0xFF);

        uint8_t b2 = (uint8_t)(buttons & 0xFF);
        uint8_t b3 = (uint8_t)((buttons >> 8) & 0xFF);

        transferDataFifo.push_back(b0);
        transferDataFifo.push_back(b1);
        transferDataFifo.push_back(b2);
        transferDataFifo.push_back(b3);
    }

    void idle() {
        mode = Mode::Idle;
    }

    bool isEnabled() {
        return enabled;
    }

    bool isReady() {
        //Console.WriteLine("[JOYPAD] Isready: " + (transferDataFifo.Count == 0));
        return transferDataFifo.empty();
    }

    void handleJoyPadDown(int key) {
        switch (key) {
        case GLFW_KEY_SPACE: buttons &= (uint16_t)~(buttons & 0x1); break;
        case GLFW_KEY_Z: buttons &= (uint16_t)~(buttons & 0x2); break;
        case GLFW_KEY_C: buttons &= (uint16_t)~(buttons & 0x4); break;
        case GLFW_KEY_ENTER: buttons &= (uint16_t)~(buttons & 0x8); break;
        case GLFW_KEY_UP: buttons &= (uint16_t)~(buttons & 0x10); break;
        case GLFW_KEY_RIGHT: buttons &= (uint16_t)~(buttons & 0x20); break;
        case GLFW_KEY_DOWN: buttons &= (uint16_t)~(buttons & 0x40); break;
        case GLFW_KEY_LEFT: buttons &= (uint16_t)~(buttons & 0x80); break;      
        case GLFW_KEY_X: buttons &= (uint16_t)~(buttons & (1 << 14)); break;
        case GLFW_KEY_BACKSPACE: buttons &= (uint16_t)~(buttons & (1 << 15)); break;
        case GLFW_KEY_O: buttons &= (uint16_t)~(buttons & (1 << 13)); break;
        }
    }

    void handleJoyPadUp(int key) {
        switch (key) {
        case GLFW_KEY_SPACE: buttons |= 0x1; break;
        case GLFW_KEY_Z: buttons |= 0x2; break;
        case GLFW_KEY_C: buttons |= 0x4; break;
        case GLFW_KEY_ENTER: buttons |= 0x8; break;
        case GLFW_KEY_UP: buttons |= 0x10; break;
        case GLFW_KEY_RIGHT: buttons |= 0x20; break;
        case GLFW_KEY_DOWN: buttons |= 0x40; break;
        case GLFW_KEY_LEFT: buttons |= 0x80; break;
        case GLFW_KEY_BACKSPACE: buttons |= (1 << 15); break;
        case GLFW_KEY_O: buttons |= (1 << 13); break;
        case GLFW_KEY_X: buttons |= (1 << 14); break;
        }
    }
};

class Bus;
class JOYPAD {
public:
    std::deque<uint8_t> JOY_TX_DATA; //1F801040h JOY_TX_DATA(W)
    std::deque<uint8_t> JOY_RX_DATA; //1F801040h JOY_RX_DATA(R) FIFO

    Bus* bus;

    //1F801044 JOY_STAT(R)
    bool TXreadyFlag1 = true;
    bool TXreadyFlag2 = true;
    bool RXparityError;
    bool ackInputLevel;
    bool interruptRequest;
    int baudrateTimer;

    //1F801048 JOY_MODE(R/W)
    uint32_t baudrateReloadFactor;
    uint32_t characterLength;
    bool parityEnable;
    bool parityTypeOdd;
    bool clkOutputPolarity;

    //1F80104Ah JOY_CTRL (R/W) (usually 1003h,3003h,0000h)
    bool TXenable;
    bool JoyOutput;
    bool RXenable;
    bool ack;
    bool reset;
    uint32_t RXinterruptMode;
    bool TXinterruptEnable;
    bool RXinterruptEnable;
    bool ACKinterruptEnable;
    uint32_t desiredSlotNumber;

    uint16_t JOY_BAUD;    //1F80104Eh JOY_BAUD(R/W) (usually 0088h, ie.circa 250kHz, when Factor = MUL1)

    Controller controller;

    int counter;

    bool tick();

    void reloadTimer() {
        //Console.WriteLine("RELOAD TIMER");
        baudrateTimer = (int)(JOY_BAUD * baudrateReloadFactor) & ~0x1;
    }

    template <typename T>
    void write(uint32_t addr, uint32_t value) {
        switch (addr & 0xFF) {
        case 0x40:
            //Console.WriteLine("[JOYPAD] TX DATA ENQUEUE " + value.ToString("x2"));
            JOY_TX_DATA.push_back((uint8_t)value);
            TXreadyFlag1 = true;
            TXreadyFlag2 = false;

            if (JoyOutput) {
                TXreadyFlag2 = true;

                if (desiredSlotNumber == 1) {
                    JOY_TX_DATA.pop_front();
                    JOY_RX_DATA.push_back(0xFF);
                    return;
                }

                uint8_t byte = JOY_TX_DATA.front();
                JOY_TX_DATA.pop_front();
                JOY_RX_DATA.push_back(controller.process(byte));
                //Console.WriteLine("[JOYPAD] TICK Enqueued RX response " + JOY_RX_DATA.Peek().ToString("x2"));
                //Console.ReadLine();
                ack = controller.ack;
                ackInputLevel = true;
            }

            if (ack) counter = 500;
            break;
        case 0x48:
            //Console.WriteLine("[JOYPAD] SET MODE " + value.ToString("x4"));
            setJOY_MODE(value);
            break;
        case 0x4A:
            //Console.WriteLine("[JOYPAD] SET CONTROL " + ((ushort)value).ToString("x4"));
            setJOY_CTRL(value);
            if (ack) {
                RXparityError = false;
                interruptRequest = false;
            }

            if (reset) {
                //Console.WriteLine("[JOYPAD] RESET");
                setJOY_MODE(0);
                setJOY_CTRL(0);
                JOY_BAUD = 0;

                JOY_RX_DATA.clear();
                JOY_TX_DATA.clear();

                TXreadyFlag1 = true;
                TXreadyFlag2 = true;
            }
            break;
        case 0x4E: {
            //Console.WriteLine("[JOYPAD] SET BAUD " + value.ToString("x4"));
            JOY_BAUD = (uint16_t)value;
            reloadTimer();
            break;
        }
        default:
            std::cout << "Unhandled JOYPAD Write\n"; break;
        }
    }

    void setJOY_CTRL(uint32_t value) {
        TXenable = (value & 0x1) != 0;
        JoyOutput = ((value >> 1) & 0x1) != 0;
        RXenable = ((value >> 2) & 0x1) != 0;
        ack = ((value >> 4) & 0x1) != 0;
        reset = ((value >> 6) & 0x1) != 0;
        RXinterruptMode = (value >> 8) & 0x3;
        TXinterruptEnable = ((value >> 10) & 0x1) != 0;
        RXinterruptEnable = ((value >> 11) & 0x1) != 0;
        ACKinterruptEnable = ((value >> 12) & 0x1) != 0;
        desiredSlotNumber = (value >> 13) & 0x1;
    }

    void setJOY_MODE(uint32_t value) {
        baudrateReloadFactor = value & 0x3;
        characterLength = (value >> 2) & 0x3;
        parityEnable = ((value >> 4) & 0x1) != 0;
        parityTypeOdd = ((value >> 5) & 0x1) != 0;
        clkOutputPolarity = ((value >> 8) & 0x1) != 0;
    }

    template <typename T>
    uint32_t read(uint32_t addr) {
        switch (addr & 0xFF) {
        case 0x40: {
            if (JOY_RX_DATA.empty()) {
                //Console.WriteLine("[JOYPAD] WARNING COUNT WAS 0 GET RX DATA returning 0");
                return 0xFF;
            }
            //Console.WriteLine("[JOYPAD] GET RX DATA " + JOY_RX_DATA.Peek().ToString("x2"));
            //Console.WriteLine("count" + (JOY_RX_DATA.Count - 1));
            uint8_t data = JOY_RX_DATA.front();
            JOY_RX_DATA.pop_front();
            return data;
        }
        case 0x44: {
            //Console.WriteLine("[JOYPAD] GET STAT " + getJOY_STAT().ToString("x8"));
            return getJOY_STAT();
        }
        case 0x48: {
            //Console.WriteLine("[JOYPAD] GET MODE " + getJOY_MODE().ToString("x8"));
            return getJOY_MODE();
        }
        case 0x4A: {
            //Console.WriteLine("[JOYPAD] GET CONTROL " + getJOY_CTRL().ToString("x8"));
            return getJOY_CTRL();
        }
        case 0x4E: {
            //Console.WriteLine("[JOYPAD] GET BAUD" + JOY_BAUD.ToString("x8"));
            return JOY_BAUD;
        }
        default: {
            std::cout << "[JOYPAD] Unhandled Read at 0x" << std::hex << addr << '\n';
            return 0xFFFFFFFF;
        }
        }
    }

    uint32_t getJOY_CTRL() {
        uint32_t joy_ctrl = 0;
        joy_ctrl |= TXenable ? 1u : 0u;
        joy_ctrl |= (JoyOutput ? 1u : 0u) << 1;
        joy_ctrl |= (RXenable ? 1u : 0u) << 2;
        joy_ctrl |= (ack ? 1u : 0u) << 4;
        joy_ctrl |= (reset ? 1u : 0u) << 6;
        joy_ctrl |= RXinterruptMode << 8;
        joy_ctrl |= (TXinterruptEnable ? 1u : 0u) << 10;
        joy_ctrl |= (RXinterruptEnable ? 1u : 0u) << 11;
        joy_ctrl |= (ACKinterruptEnable ? 1u : 0u) << 12;
        joy_ctrl |= desiredSlotNumber << 13;
        return joy_ctrl;
    }

    uint32_t getJOY_MODE() {
        uint32_t joy_mode = 0;
        joy_mode |= baudrateReloadFactor;
        joy_mode |= characterLength << 2;
        joy_mode |= (parityEnable ? 1u : 0u) << 4;
        joy_mode |= (parityTypeOdd ? 1u : 0u) << 5;
        joy_mode |= (clkOutputPolarity ? 1u : 0u) << 4;
        return joy_mode;
    }

    uint32_t getJOY_STAT() {
        uint32_t joy_stat = 0;
        joy_stat |= TXreadyFlag1 ? 1u : 0u;
        joy_stat |= (JOY_RX_DATA.size() > 0 ? 1u : 0u) << 1;
        joy_stat |= (TXreadyFlag2 ? 1u : 0u) << 2;
        joy_stat |= (RXparityError ? 1u : 0u) << 3;
        joy_stat |= (ackInputLevel ? 1u : 0u) << 7;
        joy_stat |= (interruptRequest ? 1u : 0u) << 9;
        joy_stat |= (uint32_t)baudrateTimer << 11;

        ackInputLevel = false;

        return joy_stat;
    }
};