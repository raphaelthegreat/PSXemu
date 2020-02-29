#include "dma.h"
#include <bitset>
#include <cpu/util.h>
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
	
}

bool DMAController::is_channel_enabled(DMAChannels channel)
{
	return (irq.raw & (1 << (16 + (uint8_t)channel))) || irq.master_enable;;
}

void DMAController::transfer_finished(DMAChannels channel)
{
	bool is_enabled = is_channel_enabled(channel);

	if (is_enabled) {

		std::bitset<32> bs(irq.raw);
		bs.set((uint8_t)channel + 24, true);
		irq.raw = bs.to_ulong();

		uint8_t all_enable = (irq.raw & 0x7F0000) >> 16;
		uint8_t all_flag = (irq.raw & 0x7F000000) >> 24;
		irq_pending = irq.force || (irq.master_enable && (all_enable & all_flag));
	}
}

void DMAController::start(DMAChannels dma_channel)
{
	DMAChannel& channel = channels[(uint32_t)dma_channel];
	if (channel.control.sync_mode == SyncType::Linked_List)
		/* Start linked list copy routine. */
		list_copy(dma_channel);
	else
		/* Start block copy routine. */
		block_copy(dma_channel);
}

void DMAController::block_copy(DMAChannels dma_channel)
{
	/* Get the channel to start the transfer. */
	DMAChannel& channel = channels[(uint32_t)dma_channel];
	
	/* Necessary data we need to start. */
	uint32_t trans_dir = channel.control.trans_dir;
	SyncType sync_mode = channel.control.sync_mode;
	uint32_t step_mode = channel.control.addr_step;

	/* Get base address of the transfer. */
	uint32_t base_addr = channel.base;
	
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
	uint32_t block_size = channel.block.block_size;
	if (sync_mode == SyncType::Request)
		block_size *= channel.block.block_count;

	/* Transfer the remaining blocks. */
	while (block_size > 0) {
		uint32_t addr = base_addr & 0x1ffffc;

		/* Select transfer source and destination. */
		switch (trans_dir) {
		case 0: {
			uint32_t data = 0;

			switch (dma_channel) {
			case DMAChannels::OTC:
				/*  */
				data = (block_size == 1 ? 0xffffff :
					   (addr - 4) & 0x1fffff);
				break;
			case DMAChannels::GPU:
				data = bus->gpu->get_read();
				break;
			default:
				printf("Unhandled DMA source channel: 0x%x\n", dma_channel);
				exit(0);
			}

			bus->write(addr, data);
			break;
		}
		case 1: {
			uint32_t command = bus->read(addr);

			/* Send command or operand to the GPU. */
			switch (dma_channel) {
			case DMAChannels::GPU:
				bus->gpu->gp0_command(command);
				break;
			default:
				printf("[Block copy] Unhandled DMA source channel: 0x%x\n", dma_channel);
				exit(0);
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
	transfer_finished(dma_channel);
}

void DMAController::list_copy(DMAChannels dma_channel)
{
	DMAChannel& channel = channels[(uint32_t)dma_channel];
	uint32_t addr = channel.base & 0x1ffffe;

	/* TODO: implement Device to Ram DMA transfer. */
	if (channel.control.trans_dir == 0) {
		printf("Not supported DMA direction!\n");
		exit(0);
	}

	/* While not reached the end. */
	while (true) {
		/* Get the list packet header. */
		ListPacket packet;
		packet.raw = bus->read(addr);
		uint32_t count = packet.size;

		if (count > 0)
			printf("Packet size: %d\n", count);

		/* Read words of the packet. */
		while (count > 0) {
			/* Point to next packet address. */
			addr = (addr + 4) & 0x1ffffc;
			
			/* Get command from main RAM. */
			uint32_t command = bus->read(addr);
			printf("GPU command: 0x%x\n", command);
			
			/* Send data to the GPU. */
			bus->gpu->gp0_command(command);
			count--;
		}

		/* If address is 0xffffff then we are done. */
		/* NOTE: mednafen only checks for the MSB, but I do no know why. */
		if (get_bit(packet.next_addr, 23))
			break;

		/* Mask address. */
		addr = packet.next_addr & 0x1ffffc;
	}

	/* Complete DMA Transfer */
	channel.control.enable = false;
	channel.control.trigger = false;
	transfer_finished(dma_channel);
}

uint32_t DMAController::read(uint32_t off)
{
	/* Get channel information from address. */
	uint32_t channel_num = (off & 0x70) >> 4;
	uint32_t offset = off & 0xf;

	/* One of the main channels is enabled. */
	if (channel_num >= 0 && channel_num <= 6) {
		DMAChannel& channel = channels[channel_num];

		switch (offset) {
		case 0:
			return channel.base;
		case 4:
			return channel.block.raw;
		case 8:
			return channel.control.raw;
		default:
			printf("Unhandled DMA read at offset: 0x%x\n", off);
			exit(0);
		}
	} /* One of the primary registers is selected. */
	else if (channel_num == 7) {

		switch (offset) {
		case 0:
			return control;
		case 4:
			return irq.raw;
		default:
			printf("Unhandled DMA read at offset: 0x%x\n", off);
			exit(0);
		}
	}

	return 0;
}

void DMAController::write(uint32_t off, uint32_t val)
{
	/* Get channel information from address. */
	uint32_t channel_num = (off & 0x70) >> 4;
	uint32_t offset = off & 0xf;

	uint32_t active_channel = INT_MAX;
	/* One of the main channels is enabled. */
	if (channel_num >= 0 && channel_num <= 6) {
		DMAChannel& channel = channels[channel_num];

		switch (offset) {
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
			printf("Unhandled DMA channel write at offset: 0x%x\n", off);
			exit(0);
		}

		/* Check if the channel was just activated. */
		bool trigger = true;
		if (channel.control.sync_mode == SyncType::Manual)
			trigger = channel.control.trigger;

		if (channel.control.enable && trigger)
			active_channel = channel_num;
	}/* One of the primary registers is selected. */
	else if (channel_num == 7) {

		switch (offset) {
		case 0:
			control = val;
			break;
		case 4:
			irq.raw = val;
			break;
		default:
			printf("Unhandled DMA write at offset: 0x%x\n", off);
			exit(0);
		}
	}

	/* Start DMA if a channel was just activated. */
	if (active_channel != INT_MAX)
		start((DMAChannels)active_channel);
}
