#include <stdafx.hpp>
#include "dma.h"
#include <video/gpu_core.h>

#include <memory/bus.h>

/* DMA Controller class implementation. */
DMAController::DMAController(Bus* _bus)
{
	control = 0x07654321;
	irq.raw = 0;
	bus = _bus;
}

void DMAController::tick()
{
	if (irq_pending) {
		irq_pending = false;
		bus->irq(Interrupt::DMA);
	}
}

void DMAController::transfer_finished(DMAChannels dma_channel)
{
	DMAChannel& channel = channels[(int)dma_channel];

	/* IRQ flags in Bit(24+n) are set upon DMAn completion -
	but caution - they are set ONLY if enabled in Bit(16+n).*/
	if (util::get_bit(irq.enable, (int)dma_channel) || irq.master_enable)
		irq.flags = util::set_bit(irq.flags, (int)dma_channel, true);

	/* The master flag is a simple readonly flag that follows the following rules:
	   IF b15=1 OR (b23=1 AND (b16-22 AND b24-30)>0) THEN b31=1 ELSE b31=0
	   Upon 0-to-1 transition of Bit31, the IRQ3 flag (in Port 1F801070h) gets set.
	   Bit24-30 are acknowledged (reset to zero) when writing a "1" to that bits */
	bool previous = irq.master_flag;
	irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));

	if (irq.master_flag && !previous) {
		irq_pending = true;
	}
}

void DMAController::start(DMAChannels dma_channel)
{
	DMAChannel& channel = channels[(uint)dma_channel];
	if (channel.control.sync_mode == SyncType::Linked_List)
		/* Start linked list copy routine. */
		list_copy(dma_channel);
	else
		/* Start block copy routine. */
		block_copy(dma_channel);

	/* Complete the transfer. */
	transfer_finished(dma_channel);
}

void DMAController::block_copy(DMAChannels dma_channel)
{
	/* Get the channel to start the transfer. */
	DMAChannel& channel = channels[(uint)dma_channel];

	/* Necessary data we need to start. */
	uint trans_dir = channel.control.trans_dir;
	SyncType sync_mode = channel.control.sync_mode;
	uint step_mode = channel.control.addr_step;

	/* Get base address of the transfer. */
	uint base_addr = channel.base;

	/* Set steping mode (increment or decrement at every step). */
	int32_t increment = 0;
	switch (step_mode) {
	case 0:
		increment = 4;
		break;
	case 1:
		increment = -4;
		break;
	}

	/* Get the amout of bytes each data must be interpreted as. */
	/* This is different between sync modes.
	   For Manual SyncMode:
			bits 0-15 -> number of blocks (block_size)
			bis 16-31 -> unused
	   For Request SyncMode:
			bits 0-15 -> size of each block (block_size)
			bits 16-31 -> number of blocks  (block_count)
			(So we need to do block_size * block_count to get total number of bytes)
	   For Linked List SyncMode
			bits 0-31 -> unused
	*/
	uint block_size = channel.block.block_size;
	if (sync_mode == SyncType::Request)
		block_size *= channel.block.block_count;

	/* Transfer the remaining blocks. */
	while (block_size > 0) {
		uint addr = base_addr & 0x1ffffc;

		/* Select transfer source and destination. */
		switch (trans_dir) {
		case 0: {
			uint data = 0;

			switch (dma_channel) {
			case DMAChannels::OTC:
				data = (block_size == 1 ? 0xffffff :
					(addr - 4) & 0x1fffff);
				break;
			case DMAChannels::GPU:
				data = bus->gpu->get_gpuread();
				break;
			case DMAChannels::CDROM:
				data = bus->cddrive->read_word();
				break;
			default:
				printf("Unhandled DMA source channel: 0x%x\n", dma_channel);
				__debugbreak();
			}

			bus->write(addr, data);
			break;
		}
		case 1: {
			uint command = bus->read(addr);

			/* Send command or operand to the GPU. */
			switch (dma_channel) {
			case DMAChannels::GPU:
				bus->gpu->write_gp0(command);
				break;
			default:
				//printf("[Block copy] Unhandled DMA source channel: 0x%x\n", dma_channel);
				break;
			}
			break;
		}
		}

		/* Step to the next block. */
		base_addr += increment;
		/* Decrement the remaing blocks. */
		block_size--;
	}

	/* Complete DMA Transfer */
	channel.control.enable = false;
	channel.control.trigger = false;
}

void DMAController::list_copy(DMAChannels dma_channel)
{
	DMAChannel& channel = channels[(uint)dma_channel];
	uint addr = channel.base & 0x1ffffe;

	/* TODO: implement Device to Ram DMA transfer. */
	if (channel.control.trans_dir == 0) {
		printf("Not supported DMA direction!\n");
		__debugbreak();
	}

	/* While not reached the end. */
	while (true) {
		/* Get the list packet header. */
		ListPacket packet;
		packet.raw = bus->read(addr);
		uint count = packet.size;

		/*if (count > 0)
			printf("Packet size: %d\n", count);*/

			/* Read words of the packet. */
		while (count > 0) {
			/* Point to next packet address. */
			addr = (addr + 4) & 0x1ffffc;

			/* Get command from main RAM. */
			uint command = bus->read(addr);

			/* Send data to the GPU. */
			bus->gpu->write_gp0(command);
			count--;
		}

		/* If address is 0xffffff then we are done. */
		/* NOTE: mednafen only checks for the MSB, but I do no know why. */
		if (util::get_bit(packet.next_addr, 23))
			break;

		/* Mask address. */
		addr = packet.next_addr & 0x1ffffc;
	}

	/* Complete DMA Transfer */
	channel.control.enable = false;
	channel.control.trigger = false;
}

uint DMAController::read(uint address)
{
	uint offset = bus->DMA_RANGE.offset(address);

	/* Get channel information from address. */
	uint channel_num = (offset & 0x70) >> 4;
	uint reg = offset & 0xf;

	/* One of the main channels is enabled. */
	if (channel_num >= 0 && channel_num <= 6) {
		DMAChannel& channel = channels[channel_num];

		switch (reg) {
		case 0:
			return channel.base;
		case 4:
			return channel.block.raw;
		case 8:
			return channel.control.raw;
		default:
			printf("[DMA] DMAController::read: unhandled channel read at offset: 0x%x\n", offset);
			exit(1);
		}
	} /* One of the primary registers is selected. */
	else if (channel_num == 7) {

		switch (reg) {
		case 0:
			return control;
		case 4:
			return irq.raw;
		default:
			printf("[DMA] DMAController::read: unhandled read at offset: 0x%x\n", offset);
			exit(1);
		}
	}

	return 0;
}

void DMAController::write(uint address, uint val)
{
	uint offset = bus->DMA_RANGE.offset(address);

	/* Get channel information from address. */
	uint channel_num = (offset & 0x70) >> 4;
	uint reg = offset & 0xf;

	uint active_channel = INT_MAX;
	/* One of the main channels is enabled. */
	if (channel_num >= 0 && channel_num <= 6) {
		DMAChannel& channel = channels[channel_num];

		switch (reg) {
		case 0:
			channel.base = val & 0xffffff;
			break;
		case 4:
			channel.block.raw = val;
			break;
		case 8:
			channel.control.raw = val;
			break;
		default:
			printf("[DMA] DMAController::write: unhandled channel write at offset: 0x%x\n", offset);
			exit(1);
		}

		/* Check if the channel was just activated. */
		bool trigger = true;
		if (channel.control.sync_mode == SyncType::Manual)
			trigger = channel.control.trigger;

		if (channel.control.enable && trigger)
			active_channel = channel_num;
	}/* One of the primary registers is selected. */
	else if (channel_num == 7) {

		switch (reg) {
		case 0:
			control = val;
			break;
		case 4:
			irq.raw = val;
			irq.master_flag = irq.force || (irq.master_enable && ((irq.enable & irq.flags) > 0));
			break;
		default:
			printf("[DMA] DMAController::write: unhandled write at offset: 0x%x\n", offset);
			exit(1);
		}
	}

	/* Start DMA if a channel was just activated. */
	if (active_channel != INT_MAX)
		start((DMAChannels)active_channel);
}