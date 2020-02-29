#pragma once
#include <cstdint>

enum class SyncType : uint32_t {
	Manual = 0,
	Request = 1,
	Linked_List = 2
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

/* General info about DMA for each channel. */
union DMAControlReg {
	uint32_t raw;

	struct {
		uint32_t trans_dir : 1;
		uint32_t addr_step : 1;
		uint32_t reserved : 6;
		uint32_t chop_enable : 1;
		SyncType sync_mode : 2;
		uint32_t reserved2 : 5;
		uint32_t chop_dma : 3;
		uint32_t reserved3 : 1;
		uint32_t chop_cpu : 3;
		uint32_t reserved4 : 1;
		uint32_t enable : 1;
		uint32_t reserved5 : 3;
		uint32_t trigger : 1;
		uint32_t unknown : 3;
	};
};

/* Holds info about block size and count. */
union DMABlockReg {
	uint32_t raw;

	struct {
		uint16_t block_size;
		uint16_t block_count; /* Only used in Request sync mode. */
	};
};

typedef uint32_t DMAMemReg;

/* A DMA channel */
struct DMAChannel {
	DMAControlReg control;
	DMABlockReg block;
	DMAMemReg base;
};

/* DMA Interrupt Register. */
union DMAIRQReg {
	uint32_t raw;

	struct {
		uint32_t unknown : 6;
		uint32_t not_used : 9;
		uint32_t force : 1;
		uint32_t enable : 7;
		uint32_t master_enable : 1;
		uint32_t flags : 7;
		uint32_t master_flag : 1;
	};
};

typedef uint32_t DMAControl;

union ListPacket {
	uint32_t raw;

	struct {
		uint32_t next_addr : 24;
		uint32_t size : 8;
	};
};

/* A class that manages all DMA routines. */
class Bus;
class DMAController {
public:
	DMAController(Bus* bus);

	void tick();
	bool is_channel_enabled(DMAChannels channel);
	void transfer_finished(DMAChannels channel);

	void start(DMAChannels channel);
	void block_copy(DMAChannels channel);
	void list_copy(DMAChannels channel);

	uint32_t read(uint32_t offset);
	void write(uint32_t offset, uint32_t data);

public:
	DMAControl control;
	DMAIRQReg irq;
	DMAChannel channels[7];

	bool irq_pending = false;

	Bus* bus;
};