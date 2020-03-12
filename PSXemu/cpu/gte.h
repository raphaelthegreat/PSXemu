#pragma once
#include <unordered_map>
#include <functional>
#include <iostream>
#include <glm/gtc/matrix_integer.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include "instr.h"

namespace glm {
	typedef mat<3, 3, int16_t> i16mat3;
}

union Color {
	uint32_t val;
	glm::u8vec4 color;
	struct { uint8_t r, g, b, c; };
};

union Vec2i16 {
	uint32_t val;
	struct { int16_t x, y; };
	glm::i16vec2 vector;

	auto& operator[](int idx) { return vector[idx]; }
};

union Vec3i16 {
	uint32_t xy;
	struct { int16_t x, y, z; };
	glm::i16vec3 vector;

	auto& operator[](int idx) { return vector[idx]; }
};

struct Matrix {
	Vec3i16 v1, v2, v3;
};

/* GTE Flag register. */

//union GTEControl {
//	uint32_t reg[32] = {};
//
//	struct {
//		Matrix rotation;
//		glm::ivec3 translation;
//		Matrix light_src;
//		glm::ivec3 bg_color;
//		Matrix light_color;
//		glm::ivec3 far_color;
//		glm::ivec2 screen_offset;
//		uint16_t proj_dist; uint16_t : 16;
//		uint32_t dqa;
//		uint32_t dqb;
//		int16_t zsf3;
//		int16_t : 16; /* Padding */
//		int16_t zsf4;
//		int16_t : 16; /* Padding */
//		FLAG flag;
//	};
//};
//
//union GTEData {
//	uint32_t reg[32] = {};
//
//	struct {
//		Vec3i16 vec0, vec1, vec2;
//		Color color;
//		uint16_t otz; uint16_t : 16; /* Padding */
//		int16_t ir0; uint16_t : 16; /* Padding */
//		int16_t ir1; int16_t : 16; /* Padding */
//		int16_t ir2; int16_t : 16; /* Padding */
//		int16_t ir3; int16_t : 16; /* Padding */
//		glm::vec<4, IntXY> sxy;
//		uint16_t sz0; uint16_t : 16;
//		uint16_t sz1; uint16_t : 16;
//		uint16_t sz2; uint16_t : 16;
//		uint16_t sz3; uint16_t : 16;
//		Color rgb0, rgb1, rgb2;
//		Color res1;
//		int32_t mac0;
//		glm::ivec3 macvec;
//		uint32_t irgb : 15;
//		uint32_t : 17; /* Padding */
//		uint32_t orgb : 15;
//		uint32_t : 17; /* Padding */
//		glm::ivec2 lzc;
//	};
//};

/* GTE Command encoder. */
union GTECommand {
	uint32_t raw = 0;

	struct {
		uint32_t opcode : 6;
		uint32_t : 4;
		uint32_t lm : 1;
		uint32_t : 2;
		uint32_t mvmva_tr_vec : 2;
		uint32_t mvmva_mul_vec : 2;
		uint32_t mvmva_mul_matrix : 2;
		uint32_t sf : 1;
		uint32_t fake_opcode : 5;
		uint32_t : 7;
	};
};

typedef std::function<void()> GTEFunc;

class CPU;
class GTE {
public:
	GTE(CPU* _cpu);
	~GTE() = default;

	void register_opcodes();
	inline int32_t set_mac0_flag(int64_t value);
	inline int16_t set_mac_flag(int num, int64_t value);
	inline uint16_t set_sz3_flag(int64_t value);
	inline int16_t set_sxy_flag(int i, int32_t value);
	inline int16_t set_ir0_flag(int64_t value);
	inline int16_t set_ir_flag(int i, int32_t value, bool lm);
	inline uint8_t set_rgb(int i, int value);

	uint32_t read_data(uint32_t reg);
	uint32_t read_control(uint32_t reg);

	void write_data(uint32_t reg, uint32_t val);
	void write_control(uint32_t reg, uint32_t v);

	void interpolate(int32_t mac1, int32_t mac2, int32_t mac3);
	void execute(Instr instr);

	/* GTE commands. */
	void op_rtps(); void op_rtpt();
	void op_mvmva();
	void op_dcpl();
	void op_dpcs();
	void op_intpl();
	void op_sqr();
	void op_ncs();
	void op_ncds();
	void op_nccs();
	void op_cdp();
	void op_cc();
	void op_nclip();
	void op_avsz3(); void op_avsz4();
	void op_op();
	void op_gpf();
	void op_gpl();

public:
	CPU* cpu;
	
	Vec3i16 V[3];   //R0-1 R2-3 R4-5 s16
	Color RGBC;                     //R6
	uint16_t OTZ;                     //R7
	short IR[4];      //R8-11
	Vec2i16 SXY[4]; //R12-15 FIFO
	uint16_t SZ[4];    //R16-19 FIFO
	Color RGB[3];     //R20-22 FIFO
	uint32_t RES1;                      //R23 prohibited
	int MAC0;                       //R24
	int MAC1, MAC2, MAC3;           //R25-27
	uint16_t IRGB;//, ORGB;           //R28-29 Orgb is readonly and read by irgb
	int LZCS, LZCR;                 //R30-31

	//Control Registers
	Matrix RT, LM, LRGB;        //R32-36 R40-44 R48-52
	int TRX, TRY, TRZ;          //R37-39
	int RBK, GBK, BBK;          //R45-47
	int RFC, GFC, BFC;          //R53-55
	int OFX, OFY, DQB;          //R56 57 60
	uint16_t H;                   //R58
	short ZSF3, ZSF4, DQA;      //R61 62 59
	uint32_t FLAG;
	
	int sf;                     //Shift fraction (0 or 12)
	uint32_t MVMVA_M_Matrix;         //MVMVA Multiply Matrix    (0=Rotation. 1=Light, 2=Color, 3=Reserved)
	uint32_t MVMVA_M_Vector;         //MVMVA Multiply Vector    (0=V0, 1=V1, 2=V2, 3=IR/long)
	uint32_t MVMVA_T_Vector;         //MVMVA Translation Vector (0=TR, 1=BK, 2=FC/Bugged, 3=None)
	bool lm;                     //Saturate IR1,IR2,IR3 result (0=To -8000h..+7FFFh, 1=To 0..+7FFFh)
	uint32_t opcode;

	std::unordered_map<uint32_t, GTEFunc> lookup;
	//GTECommand command;
};