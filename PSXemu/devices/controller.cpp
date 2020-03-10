#include "controller.h"
#include <memory/bus.h>

bool JOYPAD::tick()
{
    if (counter > 0) {
        counter -= 100;
        if (counter == 0) {
            //Console.WriteLine("[IRQ] TICK Triggering JOYPAD");
            ackInputLevel = false;
            interruptRequest = true;
        }
    }

    if (interruptRequest) 
        bus->irq(Interrupt::CONTROLLER);
    
    return false;
}
