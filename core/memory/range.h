#pragma once
#include <utility/types.hpp>

struct Range {
	Range(uint begin, ulong size) :
		start(begin), length(size) {}

	inline bool contains(uint addr) const 
	{
		return (addr >= start && addr < start + length);
	}

	inline uint offset(uint addr) const 
	{
		return addr - start;
	}

public:
	uint start = 0; 
	ulong length = 0;
};