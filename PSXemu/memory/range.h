#pragma once
#include <cstdint>

struct Range {
	Range(uint32_t begin, uint32_t size) :
		start(begin), length(size) {}

	inline bool contains(uint32_t addr) const {
		return (addr >= start && addr < start + length);
	}

	inline uint32_t offset(uint32_t addr) const {
		return addr - start;
	}

public:
	uint32_t start, length;
};