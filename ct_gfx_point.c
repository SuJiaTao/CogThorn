//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_point.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"

CTCALL	CTPoint			CTPointCreate(UINT32 x, UINT32 y) {
	CTPoint rp = {
		.x = x,
		.y = y
	};
	return rp;
}

CTCALL	CTPoint			CTPointAdd(CTPoint p1, CTPoint p2) {
	CTPoint rp = {
		.x = p1.x + p2.x,
		.y = p1.y + p2.y
	};
	return rp;
}

CTCALL	CTPoint			CTPointMultiply(CTPoint p, UINT32 factor) {
	CTPoint rp = {
		.x = p.x * factor,
		.y = p.y * factor
	};
	return rp;
}

CTCALL	CTPoint			CTPointFromVector(CTVect vect) {
	CTPoint rp = {
		.x = (UINT32)(max(0, vect.x) + 0.5f),
		.y = (UINT32)(max(0, vect.y) + 0.5f)
	};
	return rp;
}

CTCALL	CTVect			CTPointToVector(CTPoint p) {
	CTVect rv = {
		.x = p.x,
		.y = p.y
	};
	return rv;
}
