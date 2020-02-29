#include "cdrom.h"
#include <memory/bus.h>

CDRom::CDRom(Bus* _bus) : bus(_bus)
{
	register_commands();
}

void CDRom::write(uint32_t offset, uint32_t data)
{
	switch (offset) {
	case 0:
		status.index = data & 3;
		break;
	case 1: /* Send command to the CDRom drive. */
		if (status.index == 0) {
			status.command = uint8_t(data);
			status.has_command = true;
		}

		break;
	case 2:
		if (status.index == 0)
			parameter_fifo.push(data);
		else if (status.index == 1)
			status.irq_enable = data;

		break;
	case 3:
		if (status.index == 0)
			;/* Request register */
		else if (status.index == 1)
			status.irq_request &= ~data;
		
		break;
	}

	//printf("CDROM write at offset 0x%x with data 0x%x\n", offset, data);
}

uint32_t CDRom::read(uint32_t offset)
{
	switch (offset) {
	case 0: { // status register
		uint32_t result = status.index;

		if (parameter_fifo.size() == 0) { result |= 1 << 3; }
		if (parameter_fifo.size() != 16) { result |= 1 << 4; }
		if (response_fifo.size() != 0) { result |= 1 << 5; }
		if (data_fifo.size() != 0) { result |= 1 << 6; }

		return result;
	}

	case 1: return get_response();
	case 2: return get_data();
	case 3:
		switch (status.index & 1) {
		default: return status.irq_enable;
		case  1: return status.irq_request;
		}
	}

	return 0;
}

uint8_t CDRom::get_data()
{
	uint8_t data = data_fifo.front();
	data_fifo.pop();

	return data;
}

uint8_t CDRom::get_arg()
{
	uint8_t arg = parameter_fifo.front();
	parameter_fifo.pop();

	return arg;
}

uint8_t CDRom::get_response()
{
	uint8_t response = response_fifo.front();
	response_fifo.pop();

	return response;
}

void CDRom::tick()
{
	if (second_response != nullptr) {
		second_response();
		second_response = nullptr;
	}
	
	if (status.has_command) {
		status.has_command = false;

		auto& handler = command_lookup[status.command];
		
		if (handler != nullptr) {
			handler();
		}
		else {
			//printf("Unhandled CDRom command: 0x%x\n", status.command);
			exit(0);
		}
	}
}

void CDRom::get_stat_cmd()
{
	response_fifo.push(0x02);

	status.irq_request = 3;
	bus->interruptController.set(Interrupt::CDROM);
}

void CDRom::set_loc_cmd()
{
}

void CDRom::play_cmd()
{
}

void CDRom::forward_cmd()
{
}

void CDRom::backward_cmd()
{
}

void CDRom::readn_cmd()
{
}

void CDRom::motor_on_cmd()
{
}

void CDRom::stop_cmd()
{
}

void CDRom::pause_cmd()
{
}

void CDRom::init_cmd()
{
}

void CDRom::mute_cmd()
{
}

void CDRom::demute_cmd()
{
}

void CDRom::set_filter_cmd()
{
}

void CDRom::set_mode_cmd()
{
}

void CDRom::get_param_cmd()
{
}

void CDRom::get_locl_cmd()
{
}

void CDRom::get_locp_cmd()
{
}

void CDRom::set_session_cmd()
{
}

void CDRom::get_tn_cmd()
{
}

void CDRom::get_td_cmd()
{
}

void CDRom::seekl_cmd()
{
}

void CDRom::seekp_cmd()
{
}

void CDRom::test_cmd()
{
	uint8_t arg = get_arg();
	//printf("CDRom test command: 0x%x\n", arg);
	
	switch (arg) {
	case 0x20:
		response_fifo.push(0x99);
		response_fifo.push(0x02);
		response_fifo.push(0x01);
		response_fifo.push(0xc3);

		status.irq_request = 3;
		bus->interruptController.set(Interrupt::CDROM);
		break;
	}
}

void CDRom::get_id_cmd()
{
	//printf("CDRom get id command!\n");
	response_fifo.push(0x02);

	status.irq_request = 3;
	bus->interruptController.set(Interrupt::CDROM);

	second_response = [&]() 
	{
		response_fifo.push(0x08);
		response_fifo.push(0x40);

		response_fifo.push(0x00);
		response_fifo.push(0x00);

		response_fifo.push(0x00);
		response_fifo.push(0x00);
		response_fifo.push(0x00);
		response_fifo.push(0x00);

		status.irq_request = 5;
		bus->interruptController.set(Interrupt::CDROM);
	};
}

void CDRom::reads_cmd()
{
}

void CDRom::reset_cmd()
{
}

void CDRom::getq_cmd()
{
}

void CDRom::read_toc_cmd()
{
	//printf("CDRom read toc command!\n");
}

void CDRom::video_cd_cmd()
{
}
