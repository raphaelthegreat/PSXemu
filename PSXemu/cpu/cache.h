#pragma once
#include <cstdint>
#include <cpu/instr.h>

union Address {
	uint32_t raw;
	
	struct {
		uint32_t word_alignment : 2;
		uint32_t index : 2;
		uint32_t cache_line : 8;
		uint32_t tag : 20;
	};
};

struct CacheLine {
public:
	uint32_t tag_valid();
	void set_tag_valid(uint32_t pc);
	
	uint32_t tag();
	void invalidate();

public:
	uint32_t tag_val = 0;
	Instruction instrs[4] = {};
};

union CacheControl {
	uint32_t raw;
	
	struct {
		uint32_t lock : 1;	/* Lock Mode */
		uint32_t inv : 1;		/* Invalidate Mode */
		uint32_t tag : 1;		/* Tag Test Mode */
		uint32_t ram : 1;		/* Scratchpad RAM */
		uint32_t dblksz : 2;	/* D-Cache Refill Size */
		uint32_t reserved1 : 1;
		uint32_t ds : 1;		/* Enable D-Cache */
		uint32_t iblksz : 2;	/* I-Cache Refill Size */
		uint32_t is0 : 1;		/* Enable I-Cache Set 0 */
		uint32_t is1 : 1;		/* Enable I-Cache Set 1 */
		uint32_t intp : 1;	/* Interrupt Polarity */
		uint32_t rdpri : 1;	/* Enable Read Priority */
		uint32_t nopad : 1;	/* No Wait State */
		uint32_t bgnt : 1;	/* Enable Bus Grant */
		uint32_t ldsch : 1;	/* Enable Load Scheduling */
		uint32_t nostr : 1;	/* No Streaming */
		uint32_t reserved2 : 14;
	};
};
