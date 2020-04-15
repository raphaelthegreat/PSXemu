#pragma once
#include <utility/types.hpp>
#include <memory/bus.h>

constexpr int VOICE_COUNT = 24;

/* Volume control register. */
union Volume {
	ushort value = 0;

	struct {
		ushort volume_div_2 : 15;
		ushort : 1;
	} fixed_mode;

	struct {
		ushort sweep_step : 2;
		ushort sweep_shift : 5;
		ushort : 5;
		ushort sweep_phase : 1;
		ushort sweep_direction : 1;
		ushort sweep_mode : 1;
		ushort : 1;
	} sweep_mode;
};

/* Voice register */
union VoiceReg {
	/* eight 16bit regs per voice. */
	ushort reg[8] = {};

	struct {
		/* Stereo volumes. */
		ushort volume_left;
		ushort volume_right;
		ushort adpcm_sample_rate;
		ushort adpcm_start_addr;
		ushort adsr_lower;
		ushort adsr_upper;
		ushort adsr_volume;
		ushort adsr_repeat_addr;
	};
};

union SPUStatus {
	ushort value = 0;

	struct {
		ushort spu_mode : 6;
		ushort irq9_flag : 1;
		ushort dma_rw_request : 1;
		ushort dma_w_request : 1;
		ushort dma_r_request : 1;
		ushort busy_flag : 1;
		ushort capture_buffer : 1;
		ushort : 4;
	};
};

class SPU {
public:
	SPU(Bus* _bus) :
		bus(_bus) {}
	~SPU() = default;

	template <typename T>
	T read(uint address);

	template <typename T>
	void write(uint address, T data);

public:
	/* Per voice registers. */
	VoiceReg voice_registers[VOICE_COUNT] = {};

	/* Master volume control. */
	Volume main_left, main_right;

	/* Status/Control registers. */
	SPUStatus status;

	Bus* bus;
};

template<typename T>
inline T SPU::read(uint address)
{
	int offset = bus->SPU_RANGE.offset(address);
	/* Read to SPU Status register. */
	if (offset == 0x1AE) {
		return status.value;
	}
}

template<typename T>
inline void SPU::write(uint address, T data)
{
	int offset = bus->SPU_RANGE.offset(address);
	/* Write to voice registers. */
	if (offset >= 0 && offset < 384) {
		ubyte* raw_memory = (ubyte*)voice_registers;
		util::write_memory<T>(raw_memory, offset, data);
	}
}
