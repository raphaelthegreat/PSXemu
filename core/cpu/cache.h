#pragma once
#include <cpu/instr.hpp>

union Address {
	uint raw;

	struct {
		uint word_alignment : 2;
		uint index : 2;
		uint cache_line : 8;
		uint tag : 20;
	};
};

union CacheTag {
	uint raw;

	struct {
		uint reserved : 2;
		uint index : 3;
		uint reserved2 : 7;
		uint tag : 20;
	};
};

struct CacheLine {
	CacheTag tag;
	Instr instrs[4];
};

union CacheControl {
	uint raw;

	struct {
		uint lock : 1;	/* Lock Mode */
		uint inv : 1;		/* Invalidate Mode */
		uint tag : 1;		/* Tag Test Mode */
		uint ram : 1;		/* Scratchpad RAM */
		uint dblksz : 2;	/* D-Cache Refill Size */
		uint reserved1 : 1;
		uint ds : 1;		/* Enable D-Cache */
		uint iblksz : 2;	/* I-Cache Refill Size */
		uint is0 : 1;		/* Enable I-Cache Set 0 */
		uint is1 : 1;		/* Enable I-Cache Set 1 */
		uint intp : 1;	/* Interrupt Polarity */
		uint rdpri : 1;	/* Enable Read Priority */
		uint nopad : 1;	/* No Wait State */
		uint bgnt : 1;	/* Enable Bus Grant */
		uint ldsch : 1;	/* Enable Load Scheduling */
		uint nostr : 1;	/* No Streaming */
		uint reserved2 : 14;
	};
};