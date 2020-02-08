#include "dma.h"
#include <cpu/util.h>
#include <memory/interconnect.h>

/* DMA Channel class implementation. */

DMAChannel::DMAChannel()
{
	control.enable = false;
	control.transfer_dir = TransferDir::Device_Ram;
	control.step_mode = StepMode::Increment;
	control.sync_mode = SyncType::Manual;
	control.manual_trigger = false;
	control.chop_mode = ChoppingMode::Normal;
	control.chop_cpu = 0;
	control.chop_dma = 0;
	control.unknown = 0;
	
	base = 0;
}

uint32_t DMAChannel::get_control()
{
	uint32_t val = 0;
	val = set_bit(val, 0, (bool)control.transfer_dir);
	val = set_bit(val, 1, (bool)control.step_mode);
	val = set_bit(val, 8, (bool)control.chop_mode);
	val = set_bit_range(val, 9, 11, (uint32_t)control.sync_mode);
	val = set_bit_range(val, 16, 19, control.chop_dma);
	val = set_bit_range(val, 20, 23, control.chop_cpu);
	val = set_bit(val, 24, control.enable);
	val = set_bit(val, 28, control.manual_trigger);
	val = set_bit_range(val, 29, 31, control.unknown);

	return val;
}

uint32_t DMAChannel::get_base()
{
	return base;
}

uint32_t DMAChannel::get_block()
{
	uint8_t val = 0;
	val = set_bit_range(val, 0, 16, block.block_count);
	val = set_bit_range(val, 16, 32, block.block_size);

	return val;
}

void DMAChannel::set_control(uint32_t val)
{
	control.transfer_dir = (TransferDir)get_bit(val, 0);
	control.step_mode = (StepMode)get_bit(val, 1);
	control.chop_mode = (ChoppingMode)get_bit(val, 2);
	control.sync_mode = (SyncType)bit_range(val, 9, 11);
	control.chop_dma = (uint8_t)bit_range(val, 16, 19);
	control.chop_cpu = (uint8_t)bit_range(val, 20, 23);
	control.enable = get_bit(val, 24);
	control.manual_trigger = get_bit(val, 28);
	control.unknown = (uint8_t)bit_range(val, 29, 31);
}

void DMAChannel::set_base(uint32_t val)
{
	base = val & 0xffffff;
}

bool DMAChannel::is_active()
{
	bool trigger = true;
	if (control.sync_mode == SyncType::Manual)
		trigger = control.manual_trigger;

	return control.enable && trigger;
}

void DMAChannel::done_transfer()
{
	control.enable = false;
	control.manual_trigger = false;
}

void DMAChannel::set_block(uint32_t val)
{
	block.block_size = bit_range(val, 0, 16);
	block.block_count = bit_range(val, 16, 32);
}

/* DMA Controller class implementation. */

DMAController::DMAController(Interconnect* _inter)
{
	primary_control = 0x07654321;
	inter = _inter;
}

void DMAController::start_dma(DMAChannels channel)
{
	DMAChannel _channel = get_channel(channel);
	if (_channel.control.sync_mode == SyncType::Linked_List)
		dma_list_copy(channel);
	else
		dma_block_copy(channel);
}

void DMAController::dma_block_copy(DMAChannels _channel)
{
	DMAChannel& channel = get_channel(_channel);
	TransferDir trans_dir = channel.control.transfer_dir;
	SyncType sync_mode = channel.control.sync_mode;
	StepMode step_mode = channel.control.step_mode;

	uint32_t base_addr = channel.get_base();
	
	int32_t increment = 4; 
	if (step_mode == StepMode::Decrement)
		increment = -4;

	uint32_t block_size = channel.block.block_size;
	if (sync_mode == SyncType::Request)
		block_size *= channel.block.block_count;

	while (block_size > 0) {
		uint32_t addr = base_addr & 0x1ffffc;

		if (trans_dir == TransferDir::Device_Ram) {
			uint32_t data = 0;

			if (_channel == DMAChannels::OTC) {
				if (block_size == 1) data = 0xffffff;
				else data = (addr - 4) & 0x1fffff;
			}
			else
				panic("Unhandled DMA source channel: 0x", (uint32_t)_channel);
			
			inter->write<uint32_t>(addr, data);
		}
		else {
			uint32_t data = inter->read<uint32_t>(addr);

			if (_channel == DMAChannels::GPU)
				inter->gpu->write_gp0(data);
			else
				panic("Unhandled DMA source port!", "");
		}
			
		base_addr += increment;
		block_size--;
	}
	
	// DMA is done.
	channel.done_transfer();
}

void DMAController::dma_list_copy(DMAChannels _channel)
{
	DMAChannel& channel = get_channel(_channel);
	uint32_t addr = channel.get_base() & 0x1ffffe;

	if (channel.control.transfer_dir == TransferDir::Device_Ram)
		panic("Invalid DMA direction!", "");

	if (_channel != DMAChannels::GPU)
		panic("Attempted DMA linked lists copy on channel: 0x", (uint32_t)_channel);

	while (true) {
		uint32_t header = inter->read<uint32_t>(addr);
		uint32_t count = header >> 24;

		if (count > 0) {
			log("Linked list packet size: 0x", count);
		}

		while (count > 0) {
			addr = (addr + 4) & 0x1ffffc;
			
			
			
			uint32_t command = inter->read<uint32_t>(addr);
			log("GPU command: 0x", command);
			inter->gpu->write_gp0(command);
			count--;
		}

		if ((header & 0x800000) != 0)
			break;

		addr = header & 0x1ffffc;
	}

	channel.done_transfer();
}

uint32_t DMAController::read(uint32_t off)
{
	uint32_t channel_num = (off & 0x70) >> 4;
	uint32_t offset = off & 0xf;

	if (channel_num >= 0 && channel_num <= 6) {
		DMAChannel& channel = get_channel((DMAChannels)channel_num);

		if (offset == 0)
			return channel.get_base();
		else if (offset == 4)
			return channel.get_block();
		else if (offset == 8)
			return channel.get_control();
		else
			panic("Unhandled DMA channel read at offset: 0x", off);
	}
	else if (channel_num == 7) {

		if (offset == 0)
			return this->get_control();
		else if (offset == 4)
			return this->get_irq();
		else
			panic("Unhandled DMA read at offset: 0x", off);
	}

	return 0;
}

void DMAController::write(uint32_t off, uint32_t val)
{
	uint32_t channel_num = (off & 0x70) >> 4;
	uint32_t offset = off & 0xf;

	uint32_t active_channel = INT_MAX;
	if (channel_num >= 0 && channel_num <= 6) {
		DMAChannel& channel = get_channel((DMAChannels)channel_num);

		if (offset == 0)
			channel.set_base(val);
		else if (offset == 4)
			channel.set_block(val);
		else if (offset == 8)
			channel.set_control(val);
		else
			panic("Unhandled DMA channel write at offset: 0x", off);

		if (channel.is_active()) active_channel = channel_num;
	}
	else if (channel_num == 7) {

		if (offset == 0)
			this->set_control(val);
		else if (offset == 4)
			this->set_irq(val);
		else
			panic("Unhandled DMA write at offset: 0x", off);
	}

	if (active_channel != INT_MAX)
		start_dma((DMAChannels)active_channel);
}

uint32_t DMAController::get_irq()
{
	bool status = irq.master_enable && (irq.enable && irq.flags) > 0;
	bool master_enable = irq.force || status;

	uint32_t val = 0;
	val = set_bit_range(val, 0, 6, irq.unknown);
	val = set_bit_range(val, 6, 15, irq.not_used);
	val = set_bit(val, 15, irq.force);
	val = set_bit_range(val, 16, 23, irq.enable);
	val = set_bit(val, 23, irq.master_enable);
	val = set_bit_range(val, 24, 31, irq.flags);
	val = set_bit(val, 31, irq.master_flag);

	return val;
}

uint32_t DMAController::get_control()
{
	return primary_control;
}

DMAChannel& DMAController::get_channel(DMAChannels port)
{
	return channels[(uint32_t)port];
}

void DMAController::set_irq(uint32_t val)
{
	irq.unknown = bit_range(val, 0, 6);
	irq.force = get_bit(val, 15);
	irq.enable = bit_range(val, 16, 23);
	irq.master_enable = get_bit(val, 23);
	irq.flags = bit_range(val, 24, 31);
	irq.master_flag = get_bit(val, 31);
}

void DMAController::set_control(uint32_t val)
{
	primary_control = val;
}