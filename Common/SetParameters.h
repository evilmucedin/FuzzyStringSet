#pragma once

#include "Types.h"

#include "CityHash.h"

static const size_t MASK_LEN = 39;
static const ui64 NHASHES = (1ULL << MASK_LEN);

struct TBucket
{
	ui32 m_Offset;
};

static const size_t NBUCKETS = 32768;

inline ui64 CheckerHash(const char* buffer, size_t len)
{
	static const ui64 MASK = NHASHES - 1;
	return CityHash64(buffer, len) & MASK;
}