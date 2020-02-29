#include "timer.h"
#include <cpu/cpu.h>
#include <memory/bus.h>
#include <cstdio>

uint32_t Timer::read(uint32_t offset)
{
	switch (offset) {
	case 0: /* Write to Counter value. */
		return counter;
		break;
	case 4: /* Write to Counter control. */
		return control.raw;
		break;
	case 8: /* Write to Counter target. */
		return target;
		break;
	}
}

void Timer::write(uint32_t offset, uint32_t data)
{
	switch (offset) {
	case 0: /* Write to Counter value. */
		counter = uint16_t(data);
		break;
	case 4: { /* Write to Counter control. */
		counter = 0;
		control.raw = uint16_t(data | 0x400);

		//paused = false;
		//one_shot_irq = false;
		//control.irq_request = true;

		/* Update sync modes. */
		/*if (control.sync_enable) {
			SyncMode sync = get_sync_mode();

			if (timer_id == TimerID::TMR0 || timer_id == TimerID::TMR1) {
				if (sync == SyncMode::PauseFreeRun)
					paused = true;
			}
			if (timer_id == TimerID::TMR2) {
				if (sync == SyncMode::Stop)
					paused = true;
			}
		}*/
		break;
	}
	case 8: /* Write to Counter target. */
		target = uint16_t(data);
		break;
	}
}

void Timer::init(TimerID type, Bus* _bus)
{
	timer_id = type;

	counter = 0;
	control.raw = 0;
	target = 0;

	paused = false;
	one_shot_irq = false;
	divider = 0;

	bus = _bus;
}

void Timer::fire_irq()
{
	/* Toggle the interrupt request bit or disable it. */
	if (control.irq_pulse_mode == IRQToggle::Toggle)
		control.irq_request = !control.irq_request;
	else
		control.irq_request = false;

	if (control.irq_repeat_mode == IRQRepeat::OneShot && one_shot_irq)
		return;

	if (!control.irq_request) {
		Interrupt irq = irq_type();

		bus->interruptController.set(irq);
		one_shot_irq = true;
	}

	/* Low for only a few cycles. */
	control.irq_request = true;
}

void Timer::tick()
{
	divider++;

	if (divider == 8) {
		divider = 0;
		counter++;

		if (counter == target) {
			control.reached_target = true;

			if (control.reset == ResetWhen::Target) {
				counter = 0;
			}
			
			if (control.irq_when_target) {
				control.irq_request = false;
				bus->interruptController.set(irq_type());
			}
		}
	}
}

Interrupt Timer::irq_type()
{
	/* Select interrupt time. */
	switch (timer_id) {
	case TimerID::TMR0:
		return Interrupt::TIMER0;
	case TimerID::TMR1:
		return Interrupt::TIMER1;
	case TimerID::TMR2:
		return Interrupt::TIMER2;
	}
}

ClockSrc Timer::get_clock_source()
{
	uint32_t clock_src = control.clock_source;

	/* Timer0 can have either dotclock or sysclock. */
	if (timer_id == TimerID::TMR0) {
		/* Values 1, 3 -> dotclock. Values 0, 2 -> sysclock */
		if (clock_src == 1 || clock_src == 3)
			return ClockSrc::Dotclock;
		else
			return ClockSrc::System;
	} /* Timer1 can have either hblank or sysclock. */
	else if (timer_id == TimerID::TMR1) {
		/* Values 1, 3 -> hblank. Values 0, 2 -> sysclock */
		if (clock_src == 1 || clock_src == 3)
			return ClockSrc::Hblank;
		else
			return ClockSrc::System;
	} /* Timer2 can have either syslock / 8 or sysclock. */
	else {
		/* Values 0, 1 -> sysclock. Values 2, 3 -> sysclock / 8. */
		if (clock_src == 0 || clock_src == 1)
			return ClockSrc::System;
		else
			return ClockSrc::SystemDiv8;
	}
}

SyncMode Timer::get_sync_mode()
{
	uint32_t sync = control.sync_mode;

	if (timer_id == TimerID::TMR0 || timer_id == TimerID::TMR1) {
		return (SyncMode)sync;
	}
	else {
		if (sync == 0 || sync == 3)
			return SyncMode::Stop;
		else
			return SyncMode::FreeRun;
	}
}