#pragma once
#include <cstdint>

enum class TransferDir : uint32_t {
	Ram_Device = 1,
	Device_Ram = 0
};

enum class SyncType : uint32_t {
	Manual = 0,
	Request = 1,
	Linked_List = 2
};

enum class StepMode : uint32_t {
	Increment = 0,
	Decrement = 1
};

enum class ChoppingMode : uint32_t {
	Normal = 0,
	Chopping = 1
};

enum class DMAChannels : uint32_t {
	MDECin = 0x0,
	MDECout = 0x1,
	GPU = 0x2,
	CDROM = 0x3,
	SPU = 0x4,
	PIO = 0x5,
	OTC = 0x6
};

struct DMAControlReg {
	TransferDir transfer_dir;
	StepMode step_mode;
	ChoppingMode chop_mode;
	
	SyncType sync_mode : 2;
	uint32_t chop_dma : 3;
	uint32_t chop_cpu : 3;
	
	bool enable;
	bool manual_trigger;

	uint32_t unknown : 2;
};

struct DMABlockReg {
	uint16_t block_size;
	uint16_t block_count; // Only used in Request mode
};

typedef uint32_t DMAMemReg;

// A helper class representing a DMA channel
class DMAChannel {
public:
	DMAChannel();

	uint32_t get_control();
	uint32_t get_block();
	uint32_t get_base();

	void set_control(uint32_t val);
	void set_block(uint32_t val);
	void set_base(uint32_t val);

	bool is_active();
	void done_transfer();

public:
	DMAControlReg control;
	DMABlockReg block;
	DMAMemReg base;
};

typedef uint32_t DMAPrimaryControl;

struct DMAIRQReg {
	uint32_t unknown : 6;
	uint32_t not_used : 9;
	uint32_t force : 1;
	uint32_t enable : 7;
	uint32_t master_enable : 1;
	uint32_t flags : 7;
	uint32_t master_flag : 1;
};

// A class managing all reads, writes and functions to DMA
class Interconnect;
class DMAController {
public:
	DMAController(Interconnect* inter);

	void start_dma(DMAChannels channel);
	void dma_block_copy(DMAChannels channel);
	void dma_list_copy(DMAChannels channel);

	uint32_t read(uint32_t offset);
	void write(uint32_t offset, uint32_t data);

	uint32_t get_irq();
	uint32_t get_control();
	DMAChannel& get_channel(DMAChannels channel);

	void set_irq(uint32_t val);
	void set_control(uint32_t val);
	
public:
	DMAPrimaryControl primary_control = 0;
	DMAIRQReg irq;
	
	DMAChannel channels[7];
	Interconnect* inter;
};