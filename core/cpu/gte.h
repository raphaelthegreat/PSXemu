#pragma once
#include <cpu/instr.hpp>

namespace glm {
	typedef mat<3, 3, int16_t> i16mat3;
}

union Color {
	uint val;
	glm::u8vec4 color;
	struct { ubyte r, g, b, c; };
};

union Vec2i16 {
	uint val;
	struct { int16_t x, y; };
	glm::i16vec2 vector;

	auto& operator[](int idx) { return vector[idx]; }
};

union Vec3i16 {
	uint xy;
	struct { int16_t x, y, z; };
	glm::i16vec3 vector;

	auto& operator[](int idx) { return vector[idx]; }
};

struct Matrix {
	Vec3i16 v1, v2, v3;
};

/* GTE Flag register. */

//union GTEControl {
//	uint reg[32] = {};
//
//	struct {
//		Matrix rotation;
//		glm::ivec3 translation;
//		Matrix light_src;
//		glm::ivec3 bg_color;
//		Matrix light_color;
//		glm::ivec3 far_color;
//		glm::ivec2 screen_offset;
//		ushort proj_dist; ushort : 16;
//		uint dqa;
//		uint dqb;
//		int16_t zsf3;
//		int16_t : 16; /* Padding */
//		int16_t zsf4;
//		int16_t : 16; /* Padding */
//		FLAG flag;
//	};
//};
//
//union GTEData {
//	uint reg[32] = {};
//
//	struct {
//		Vec3i16 vec0, vec1, vec2;
//		Color color;
//		ushort otz; ushort : 16; /* Padding */
//		int16_t ir0; ushort : 16; /* Padding */
//		int16_t ir1; int16_t : 16; /* Padding */
//		int16_t ir2; int16_t : 16; /* Padding */
//		int16_t ir3; int16_t : 16; /* Padding */
//		glm::vec<4, IntXY> sxy;
//		ushort sz0; ushort : 16;
//		ushort sz1; ushort : 16;
//		ushort sz2; ushort : 16;
//		ushort sz3; ushort : 16;
//		Color rgb0, rgb1, rgb2;
//		Color res1;
//		int32_t mac0;
//		glm::ivec3 macvec;
//		uint irgb : 15;
//		uint : 17; /* Padding */
//		uint orgb : 15;
//		uint : 17; /* Padding */
//		glm::ivec2 lzc;
//	};
//};

/* GTE Command encoder. */
union GTECommand {
	uint raw = 0;

	struct {
		uint opcode : 6;
		uint : 4;
		uint lm : 1;
		uint : 2;
		uint mvmva_tr_vec : 2;
		uint mvmva_mul_vec : 2;
		uint mvmva_mul_matrix : 2;
		uint sf : 1;
		uint fake_opcode : 5;
		uint : 7;
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
	inline int64_t set_mac_flag(int num, int64_t value);
	inline ushort set_sz3_flag(int64_t value);
	inline int16_t set_sxy_flag(int i, int32_t value);
	inline int16_t set_ir0_flag(int64_t value);
	inline int16_t set_ir_flag(int i, int32_t value, bool lm);
	inline ubyte set_rgb(int i, int value);

	uint read_data(uint reg);
	uint read_control(uint reg);

	void write_data(uint reg, uint val);
	void write_control(uint reg, uint v);

	void interpolate(int32_t mac1, int32_t mac2, int32_t mac3);
	void execute(Instr instr);

	Matrix get_mvmva_matrix();
	Vec3i16 get_mvmva_vector();
	std::tuple<int, int, int> get_mvmva_translation();

	/* GTE commands. */
	void op_rtps(); void op_rtpt();
	void op_mvmva();
	void op_dcpl();
	void op_dpcs();
	void op_intpl();
	void op_ncdt();
	void op_sqr();
	void op_nct();
	void op_ncs();
	void op_ncds();
	void op_nccs(); void op_ncct();
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
	ushort OTZ;                     //R7
	short IR[4];      //R8-11
	Vec2i16 SXY[4]; //R12-15 FIFO
	ushort SZ[4];    //R16-19 FIFO
	Color RGB[3];     //R20-22 FIFO
	uint RES1;                      //R23 prohibited
	int MAC0;                       //R24
	int MAC1, MAC2, MAC3;           //R25-27
	ushort IRGB;//, ORGB;           //R28-29 Orgb is readonly and read by irgb
	int LZCS, LZCR;                 //R30-31

	//Control Registers
	Matrix RT, LM, LRGB;        //R32-36 R40-44 R48-52
	int TRX, TRY, TRZ;          //R37-39
	int RBK, GBK, BBK;          //R45-47
	int RFC, GFC, BFC;          //R53-55
	int OFX, OFY, DQB;          //R56 57 60
	ushort H;                   //R58
	short ZSF3, ZSF4, DQA;      //R61 62 59
	uint FLAG;

	GTECommand command;
	std::unordered_map<uint, GTEFunc> lookup;
	//GTECommand command;
};