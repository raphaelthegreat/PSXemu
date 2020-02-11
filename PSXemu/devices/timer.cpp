#include "timer.h"
#include <cpu/cpu.h>
#include <memory/bus.h>
#include <cstdio>

Timer::Timer(TimerID type, Bus* _bus)
{
	timer_id = type;

	current.raw = 0;
	mode.raw = 0;
	target.raw = 0;

	paused = false;
	one_shot_irq = false;
	count = 0;

	bus = _bus;
}

uint32_t Timer::read(uint32_t offset)
{
	printf("Read to timer: %d at offset: %d\n", timer_id, offset);
	
	switch (offset) {
	case 0: /* Write to Counter value. */
		return current.raw;
		break;
	case 4: /* Write to Counter control. */
		return mode.raw;
	case 8: /* Write to Counter target. */
		return target.raw;
	}
}

void Timer::write(uint32_t offset, uint32_t data)
{
	printf("Write to timer: %d at offset: %d with data: 0x%x\n", timer_id, offset, data);

	switch (offset) {
	case 0: /* Write to Counter value. */
		current.raw = data;
		break;
	case 4: { /* Write to Counter control. */
		current.raw = 0;
		mode.raw = data;

		paused = false;
		one_shot_irq = false;
		mode.irq_request = true;
			
		printf("Update timer sync mode!\n");

		/* Update sync modes. */
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
	}
	case 8: /* Write to Counter target. */
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

	if (mode.irq_repeat_mode == IRQRepeat::OneShot && one_shot_irq)
		return;

	if (!mode.irq_request) {
		Interrupt irq = irq_type();

		bus->irq_manager.irq_request(irq);
		one_shot_irq = true;
	}

	exit(0);

	/* Low for only a few cycles. */
	mode.irq_request = true;
}

void Timer::tick(uint32_t cycles)
{
	/* Dont do anything if paused. */
	if (paused)
		return;

	/* Get the timer's clock source. */
	ClockSrc clock_src = get_clock_source();

	/* Add the cycles to the counter. */
	count += cycles;
	
	/* Increment timer according to the clock source and id. */
	if (timer_id == TimerID::TMR0) {
		if (clock_src == ClockSrc::Dotclock) {
			current.raw += count / 6;
			count %= 6;
		}
		else {
			current.raw += (uint32_t)(count / 1.5f);
			count %= (uint32_t)1.5f;
		}
	}
	else if (timer_id == TimerID::TMR1) {
		if (clock_src == ClockSrc::Hblank) {
			current.raw += count / 3413;
			count %= 3413;
		}
		else {
			current.raw += (uint32_t)(count / 1.5f);
			count %= (uint32_t)1.5f;
		}
	}
	else if (timer_id == TimerID::TMR2) {
		if (clock_src == ClockSrc::SystemDiv8) {
			current.raw += (uint32_t)(count / (8 * 1.5f));
			count %= (uint32_t)(8 * 1.5f);
		}
		else {
			current.raw += (uint32_t)(count * 1.5f);
			count %= (uint32_t)1.5f;
		}
	}

	bool should_irq = false;

	/* If we reached or exceeded the target value. */
	if (current.value >= target.target) {
		mode.reached_target = true;
		
		if (mode.reset == ResetWhen::Target) 
			current.raw = 0;
		
		if (mode.irq_when_target) 
			should_irq = true;
	}

	/* If there was an onverflow. */
	if (current.raw >= 0xffff) {
		mode.reached_overflow = true;

		if (mode.reset == ResetWhen::Overflow) 
			current.raw = 0;
		if (mode.irq_when_overflow) 
			should_irq = true;
	}

	/* Fire interrupt if necessary. */
	if (should_irq) 
		fire_irq();
}

Interrupt Timer::irq_type()
{
	/* Select interrupt time. */
	switch (timer_id) {
	case TimerID::TMR0: 
		return Interrupt::TMR0;
	case TimerID::TMR1:
		return Interrupt::TMR1;
	case TimerID::TMR2:
		return Interrupt::TMR2;
	}
}

ClockSrc Timer::get_clock_source()
{
	uint32_t clock_src = mode.clock_source;

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
	uint32_t sync = mode.sync_mode;
	
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
