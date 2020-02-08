#pragma once
#include <cstdint>

struct CacheControl {
	bool scartchpad_enable;
	bool unknown3;
	bool scartchpad_enable2;
	bool unknown4;
	bool crash;
	bool unknown5;
	bool code_cache_enable;
};

struct CachedInstr {
	uint32_t value = 0;
	bool active = false;
};

struct CacheLine {
	uint32_t tag = 0;
	CachedInstr instrs[4];
};

class InstrCache {
public:
	InstrCache() = default;

private:
	CacheLine cache[256];
};