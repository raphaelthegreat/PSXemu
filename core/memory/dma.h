#pragma once
#include <memory/range.h>

enum class SyncType : uint {
	Manual = 0,
	Request = 1,
	Linked_List = 2
};

enum class DMAChannels : uint {
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
	uint raw;

	struct {
		uint trans_dir : 1;
		uint addr_step : 1;
		uint reserved : 6;
		uint chop_enable : 1;
		SyncType sync_mode : 2;
		uint reserved2 : 5;
		uint chop_dma : 3;
		uint reserved3 : 1;
		uint chop_cpu : 3;
		uint reserved4 : 1;
		uint enable : 1;
		uint reserved5 : 3;
		uint trigger : 1;
		uint unknown : 3;
	};
};

/* Holds info about block size and count. */
union DMABlockReg {
	uint raw;

	struct {
		ushort block_size;
		ushort block_count; /* Only used in Request sync mode. */
	};
};

typedef uint DMAMemReg;

/* A DMA channel */
struct DMAChannel {
	DMAControlReg control;
	DMABlockReg block;
	DMAMemReg base;
};

/* DMA Interrupt Register. */
union DMAIRQReg {
	uint raw;

	struct {
		uint unknown : 6;
		uint not_used : 9;
		uint force : 1;
		uint enable : 7;
		uint master_enable : 1;
		uint flags : 7;
		uint master_flag : 1;
	};
};

typedef uint DMAControl;

union ListPacket {
	uint raw;

	struct {
		uint next_addr : 24;
		uint size : 8;
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

	uint read(uint address);
	void write(uint address, uint data);

public:
	DMAControl control;
	DMAIRQReg irq;
	DMAChannel channels[7];

	bool irq_pending = false;
	Bus* bus;
};