#pragma once
#include <utility/types.hpp>
#include <memory/bus.h>

constexpr int VOICE_COUNT = 24;

/* Volume control register. */
union Volume {
	ushort value = 0;
	ubyte bytes[2];

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
	ubyte bytes[2];

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

union SPUControl {
	ushort value = 0;
	ubyte bytes[2];

	struct {
		ushort cd_audio_enable : 1;
		ushort external_audio_enable : 1;
		ushort cd_audio_reverb : 1;
		ushort external_audio_reverb : 1;
		ushort sound_ram_transfer_mode : 2;
		ushort irq9_enable : 1;
		ushort reverb_master_enable : 1;
		ushort noise_freq_step : 2;
		ushort noise_freq_shift : 4;
		ushort mute_spu : 1;
		ushort spu_enable : 1;
	};
};

/* SPU Reverb Registers. */
union ReverbRegs {
	ushort reg[32];

	struct {
		ushort dAPF1;	/* Reverb APF Offset 1. */
		ushort dAPF2;	/* Reverb APF Offset 2. */
		ushort vIIR;	/* Reverb Reflection Volume 1. */
		ushort vCOMB1;	/* Reverb Comb Volume 1. */
		ushort vCOMB2;	/* Reverb Comb Volume 2. */
		ushort vCOMB3;	/* Reverb Comb Volume 3. */
		ushort vCOMB4;	/* Reverb Comb Volume 4. */
		ushort vWALL;	/* Reverb Reflection Volume 2. */
		ushort vAPF1;	/* Reverb APF Volume 1. */
		ushort vAPF2;	/* Reverb APF Volume 2. */
		ushort mLSAME;	/* Reverb Same Side Reflection Address 1 Left. */
		ushort mRSAME;	/* Reverb Same Side Reflection Address 1 Right. */
		ushort mLCOMB1; /* Reverb Comb Address 1 Left. */
		ushort mRCOMB1; /* Reverb Comb Address 1 Right. */
		ushort mLCOMB2; /* Reverb Comb Address 2 Left. */
		ushort mRCOMB2; /* Reverb Comb Address 2 Right. */
		ushort dLSAME;	/* Reverb Same Side Reflection Address 2 Left. */
		ushort dRSAME;	/* Reverb Same Side Reflection Address 2 Right. */
		ushort mLDIFF;	/* Reverb Different Side Reflect Address 1 Left. */
		ushort mRDIFF;	/* Reverb Different Side Reflect Address 1 Right. */
		ushort mLCOMB3; /* Reverb Comb Address 3 Left. */
		ushort mRCOMB3; /* Reverb Comb Address 3 Right. */
		ushort mLCOMB4; /* Reverb Comb Address 4 Left. */
		ushort mRCOMB4; /* Reverb Comb Address 4 Right. */
		ushort dLDIFF;	/* Reverb Different Side Reflect Address 2 Left. */
		ushort dRDIFF;	/* Reverb Different Side Reflect Address 2 Right. */
		ushort mLAPF1;	/* Reverb APF Address 1 Left. */
		ushort mRAPF1;	/* Reverb APF Address 1 Right. */
		ushort mLAPF2;	/* Reverb APF Address 2 Left. */
		ushort mRAPF2;	/* Reverb APF Address 2 Right. */
		ushort vLIN;	/* Reverb Input Volume Left. */
		ushort vRIN;	/* Reverb Input Volume Right. */
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
	
	/* Voice flags. */
	uint voice_key_on = 0;
	uint voice_key_off = 0;
	uint voice_on_off = 0;
	uint voice_reverb_mode = 0;
	uint voice_noise_mode_enable = 0;
	uint voice_pitch_modulation_enable = 0;

	ushort cd_volume = 0;
	ushort extern_volume = 0;
	ushort curr_main_volume = 0;
	ushort sound_ram_data_transfer_control = 0;
	ushort sound_ram_data_transfer_address = 0;
	ushort sound_ram_data_transfer_fifo = 0;
	ushort reverb_work_area_start_addr = 0;

	/* Reverb registers. */
	ushort v_lout = 0, v_rout = 0;
	ReverbRegs reverb_regs;

	/* Master volume control. */
	Volume main_left, main_right;

	/* Status/Control registers. */
	SPUStatus status;
	SPUControl control;

	Bus* bus;
};

template<typename T>
inline T SPU::read(uint address)
{
	int offset = bus->SPU_RANGE.offset(address);
	/* Read to SPU Status register. */
	if (offset >= 0 && offset < 0x180) {
		return util::read_memory<T>(voice_registers, offset);
	} else if (offset >= 0x1AA && offset < 0x1AC) {
		return util::read_memory<T>(control.bytes, offset - 0x1AA);
	} else if (offset >= 0x1AE && offset < 0x1B0) {
		return util::read_memory<T>(status.bytes, offset - 0x1AE);
	} else if (offset >= 0x188 && offset < 0x18C) {
		return util::read_memory<T>(&voice_key_on, offset - 0x188);
	} else if (offset >= 0x18C && offset < 0x190) {
		return util::read_memory<T>(&voice_key_off, offset - 0x18C);
	} else if (offset >= 0x1AC && offset < 0x1AE) {
	 	return util::read_memory<T>(&sound_ram_data_transfer_control, offset - 0x1AC);
	} else if (offset >= 0x1B8 && offset < 0x1BC) {
		return util::read_memory<T>(&curr_main_volume, offset - 0x1B8);
	} else if (offset >= 0x1A6 && offset < 0x1A8) {
		return util::read_memory<T>(&sound_ram_data_transfer_address, offset - 0x1A6);
	} else if (offset >= 0x198 && offset < 0x19C) {
		return util::read_memory<T>(&voice_reverb_mode, offset - 0x198);
	} else {
		return 0;
	}
}

template<typename T>
inline void SPU::write(uint address, T data)
{
	int offset = bus->SPU_RANGE.offset(address);
	/* Write to voice registers. */
	if (offset >= 0 && offset < 0x180) {
		util::write_memory<T>(voice_registers, offset, data);
	} else if (offset >= 0x180 && offset < 0x182) {
		util::write_memory<T>(main_left.bytes, offset - 0x180, data);
	} else if (offset >= 0x182 && offset < 0x184) {
		util::write_memory<T>(main_right.bytes, offset - 0x182, data);
	} else if (offset >= 0x184 && offset < 0x186) {
		util::write_memory<T>(&v_lout, offset - 0x184, data);
	} else if (offset >= 0x186 && offset < 0x188) {
		util::write_memory<T>(&v_rout, offset - 0x186, data);
	} else if (offset >= 0x188 && offset < 0x18C) {
		util::write_memory<T>(&voice_key_on, offset - 0x188, data);
	} else if (offset >= 0x1AA && offset < 0x1AC) {
		util::write_memory<T>(control.bytes, offset - 0x1AA, data);
	} else if (offset >= 0x18C && offset < 0x190) {
		util::write_memory<T>(&voice_key_off, offset - 0x18C, data);
	} else if (offset >= 0x190 && offset < 0x194) {
		util::write_memory<T>(&voice_pitch_modulation_enable, offset - 0x190, data);
	} else if (offset >= 0x19C && offset < 0x1A0) {
		util::write_memory<T>(&voice_on_off, offset - 0x19C, data);
	} else if (offset >= 0x194 && offset < 0x198) {
		util::write_memory<T>(&voice_noise_mode_enable, offset - 0x194, data);
	} else if (offset >= 0x198 && offset < 0x19C) {
		util::write_memory<T>(&voice_reverb_mode, offset - 0x198, data);
	} else if (offset >= 0x1B0 && offset < 0x1B4) {
		util::write_memory<T>(&cd_volume, offset - 0x1B0, data);
	} else if (offset >= 0x1B4 && offset < 0x1B8) {
		util::write_memory<T>(&extern_volume, offset - 0x1B4, data);
	} else if (offset >= 0x1AC && offset < 0x1AE) {
		util::write_memory<T>(&sound_ram_data_transfer_control, offset - 0x1AC, data);
	} else if (offset >= 0x1A6 && offset < 0x1A8) {
		util::write_memory<T>(&sound_ram_data_transfer_address, offset - 0x1A6, data);
	} else if (offset >= 0x1A8 && offset < 0x1AA) {
		/* Unimplemented. */
	} else if (offset >= 0x1A2 && offset < 0x1A4) {
		util::write_memory<T>(&reverb_work_area_start_addr, offset - 0x1A2, data);
	} else if (offset >= 0x1C0 && offset < 0x200) {
		util::write_memory<T>(reverb_regs.reg, offset - 0x1C0, data);
	} else {
		return;
	}
}
