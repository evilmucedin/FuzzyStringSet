#pragma once

#include "Types.h"

static const size_t MASK_LEN = 39;
static const ui64 NHASHES = (1ULL << MASK_LEN);
static const ui64 MASK = NHASHES - 1;

struct TBucket
{
	ui32 m_Offset;
};

static const size_t NBUCKETS = 10000;