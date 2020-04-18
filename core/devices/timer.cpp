#include <stdafx.hpp>
#include <memory/bus.h>
#include "timer.h"
#include <cpu/cpu.h>

Timer::Timer(TimerID type, Bus* _bus)
{
	timer_id = type;

	current.raw = 0;
	mode.raw = 0;
	target.raw = 0;

	paused = false;
	already_fired_irq = false;

	count = 0;
	bus = _bus;
}

uint Timer::read(uint address)
{
	switch (address & 0xf) {
	case 0x0: /* Write to Counter value. */
		return current.value;
	case 0x4: /* Write to Counter control. */
		return mode.raw;
	case 0x8: /* Write to Counter target. */
		return target.target;
	default:
		exit(0);
	}
}

void Timer::write(uint address, uint data)
{
	switch (address & 0xf) {
	case 0x0: /* Write to Counter value. */
		current.raw = data;
		break;
	case 0x4: { /* Write to Counter control. */
		current.raw = 0;
		mode.raw = data;

		already_fired_irq = false;
		mode.irq_request = true;

		if (mode.sync_enable) {
			SyncMode sync = get_sync_mode();

			if (timer_id == TimerID::TMR0 || timer_id == TimerID::TMR1) {
				if (sync == SyncMode::PauseFreeRun)
					paused = true;
			}
			if (timer_id == TimerID::TMR2) {
				if (sync == SyncMode::Stop)
					paused = true;
			}
		}
		break;
	}
	case 0x8: /* Write to Counter target. */
		target.raw = data;
		break;
	}
}

void Timer::fire_irq()
{
	/* Toggle the interrupt request bit or disable it. */
	if (mode.irq_pulse_mode == IRQToggle::Toggle)
		mode.irq_request = !mode.irq_request;
	else
		mode.irq_request = false;

	bool trigger = !mode.irq_request;
	if (mode.irq_repeat_mode == IRQRepeat::OneShot) {
		if (!already_fired_irq && trigger)
			already_fired_irq = true;
		else
			return;
	}
	
	mode.irq_request = true;
	Interrupt type = irq_type();
	bus->irq(type);
}

void Timer::tick(uint cycles)
{
	/* Get the timer's clock source. */
	ClockSrc clock_src = get_clock_source();

	/* Get the timer's sync mode. */
	SyncMode sync = get_sync_mode();

	/* Add the cycles to the counter. */
	count += cycles;

	/* Increment timer according to the clock source and id. */
	if (timer_id == TimerID::TMR0) {
		if (mode.sync_enable) {
			/* Timer is paused. */
			if (sync == SyncMode::Pause && in_hblank) {
				return;
			}
			/* Timer must be reset. */
			else if (sync == SyncMode::Reset && in_hblank) {
				current.raw = 0;
			}
			/* Timer must be reset and paused outside of hblank. */
			else if (sync == SyncMode::ResetPause) {
				if (in_hblank) current.raw = 0;
				else return;
			}
			/* Timer is paused until hblank occurs once. */
			else {
				if (!prev_hblank && in_hblank) mode.sync_enable = false;
				else return;
			}
		}

		if (clock_src == ClockSrc::Dotclock) {
			current.raw += (ushort)(count * 11 / 7 / dot_div);
			count = 0;
		}
		else {
			current.raw += (ushort)count;
			count = 0;
		}
	}
	else if (timer_id == TimerID::TMR1) {
		if (mode.sync_enable) {
			/* Timer is paused. */
			if (sync == SyncMode::Pause && in_vblank) {
				return;
			}
			/* Timer must be reset. */
			else if (sync == SyncMode::Reset && in_vblank) {
				current.raw = 0;
			}
			/* Timer must be reset and paused outside of vblank. */
			else if (sync == SyncMode::ResetPause) {
				if (in_vblank) current.raw = 0;
				else return;
			}
			/* Timer is paused until vblank occurs once. */
			else {
				if (!prev_vblank && in_vblank) mode.sync_enable = false;
				else return;
			}
		}

		if (clock_src == ClockSrc::Hblank) {
			current.raw += (ushort)(count / 2160);
			count %= 2160;
		}
		else {
			current.raw += (ushort)count;
			count = 0;
		}
	}
	else if (timer_id == TimerID::TMR2) {
		if (mode.sync_enable && sync == SyncMode::Stop)
			return;

		if (clock_src == ClockSrc::SystemDiv8) {
			current.raw += (ushort)(count / 8);
			count %= 8;
		}
		else {
			current.raw += (ushort)count;
			count = 0;
		}
	}

	bool should_irq = false;
	/* If we reached or exceeded the target value. */
	if (current.raw >= target.target) {
		mode.reached_target = true;

		if (mode.reset == ResetWhen::Target)
			current.raw = 0;

		if (mode.irq_when_target)
			should_irq = true;
	}

	/* If there was an overflow. */
	if (current.raw >= 0xffff) {
		mode.reached_overflow = true;

		//if (mode.reset == ResetWhen::Overflow)
		//	current.raw = 0;

		if (mode.irq_when_overflow)
			should_irq = true;
	}

	/* Fire interrupt if necessary. */
	if (should_irq) {
		fire_irq();
	}
}

void Timer::gpu_sync(GPUSync sync)
{
	prev_hblank = in_hblank;
	prev_vblank = in_vblank;

	in_hblank = sync.hblank;
	in_vblank = sync.vblank;

	dot_div = sync.dotDiv;
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
	default:
		exit(0);
	}
}

ClockSrc Timer::get_clock_source()
{
	uint clock_src = mode.clock_source;

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
	uint sync = mode.sync_mode;

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