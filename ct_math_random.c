//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_math_random.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_math.h"

#include <math.h>
#include <intrin.h>

CTCALL	UINT32		CTRandomSeed(void) {
	// naiive timestamp approach, further randomized by sqr
	UINT64 timeStamp = __rdtsc();
	return ((timeStamp * timeStamp) & MAXUINT32);
}

CTCALL	UINT32		CTRandomInt(UINT32 seed) {
	// simple XORSHIFT
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
	return seed;
}

CTCALL	FLOAT		CTRandomFloat(FLOAT seed) {
	UINT32 iseed	= *(PUINT32)&seed;
	UINT32 randNum	= CTRandomInt(iseed);

	// constrain randNum to [0, 1023]
	randNum &= 1023;

	// normalize to [0, 1]
	FLOAT rFloat = (FLOAT)randNum;
	rFloat *= 0.0009775171065f;		// divide by 1023

	return rFloat;
}

CTCALL	BOOL		CTRandomChance(UINT32 seed, UINT32 num, UINT32 denom) {
	return (CTRandomInt(seed) % denom) < num;
}
