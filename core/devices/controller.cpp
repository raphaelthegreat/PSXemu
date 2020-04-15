#include <stdafx.hpp>
#include "controller.h"
#include <memory/bus.h>
#include <GLFW/glfw3.h>

ubyte Controller::process(ubyte byte)
{
    if (state == ControllerState::Idle) {
        if (byte == 0x1) {
            state = ControllerState::Transfer;
            enabled = true;
            ack = true;
            return 0xFF;
        }
        else {
            ack = false;
            return 0xFF;
        }
    }
    else if (state == ControllerState::Transfer) {
        
        if (byte == 0x1) {
            ack = true;
            return 0xFF;
        }
        else if (byte == 0x42) {
            state = ControllerState::Transfer;
            ack = true;
            gen_response();
            
            return dequeue();
        }
        else {
            ubyte data;            
            if (transfer_fifo.empty()) {
                enabled = false;
                state = ControllerState::Idle;
                ack = false;
                data = 0xFF;
            }
            else
                data = dequeue();
            
            return data;
        }        
    }
    else {
        ack = false;
        return 0xFF;
    }
}

void Controller::gen_response()
{
    ubyte b0 = (int)type & 0xFF;
    ubyte b1 = ((int)type >> 8) & 0xFF;

    ubyte b2 = (ubyte)(buttons & 0xFF);
    ubyte b3 = (ubyte)((buttons >> 8) & 0xFF);

    transfer_fifo.push_back(b0);
    transfer_fifo.push_back(b1);
    transfer_fifo.push_back(b2);
    transfer_fifo.push_back(b3);
}

ubyte Controller::dequeue()
{
    ubyte data = transfer_fifo.front();
    transfer_fifo.pop_front();
    return data;
}

void Controller::key_down(int key)
{
    Button button = key_lookup[key];
    buttons = util::set_bit(buttons, (int)button, false);
}

void Controller::key_up(int key)
{
    Button button = key_lookup[key];
    buttons = util::set_bit(buttons, (int)button, true);
}

ControllerManager::ControllerManager(Bus* _bus) :
    bus(_bus)
{
    status.value = 0;
    control.value = 0;
    mode.value = 0;

    status.tx_ready_flag_1 = true;
    status.tx_ready_flag_2 = true;
}

Controller::Controller()
{
    /* Use the default mapping on startup. */
    default_mapping();
}

void Controller::default_mapping()
{
    key_lookup[GLFW_KEY_CAPS_LOCK] = Button::Select;
    key_lookup[GLFW_KEY_Q] = Button::Start;
    key_lookup[GLFW_KEY_W] = Button::Up;
    key_lookup[GLFW_KEY_D] = Button::Right;
    key_lookup[GLFW_KEY_S] = Button::Down;
    key_lookup[GLFW_KEY_A] = Button::Left;
    key_lookup[GLFW_KEY_3] = Button::L2;
    key_lookup[GLFW_KEY_4] = Button::R2;
    key_lookup[GLFW_KEY_1] = Button::L1;
    key_lookup[GLFW_KEY_2] = Button::R1;
    key_lookup[GLFW_KEY_F] = Button::Trig;
    key_lookup[GLFW_KEY_G] = Button::Circle;
    key_lookup[GLFW_KEY_SPACE] = Button::Cross;
    key_lookup[GLFW_KEY_ENTER] = Button::Square;
}

bool ControllerManager::tick()
{
    if (counter > 0) {
        counter -= 100;
        if (counter == 0) {
            status.ack_input_level = false;
            status.irq_request = true;
        }
    }

    if (status.irq_request)
        bus->irq(Interrupt::CONTROLLER);

    return false;
}

void ControllerManager::reload()
{
    status.baudrate_timer = (baud * mode.reload_factor) & ~0x1;
}