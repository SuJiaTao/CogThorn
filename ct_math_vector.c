//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_math_vector.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_math.h"

#include <math.h>

CTCALL	CTVect	CTMakeVect(FLOAT x, FLOAT y) {
	CTVect rv = {
		.x = x,
		.y = y
	};
	return rv;
}

CTCALL	CTVect	CTVectAdd(CTVect v1, CTVect v2) {
	CTVect rv = {
		.x = v1.x + v2.x,
		.y = v2.y + v2.y
	};
	return rv;
}

CTCALL	CTVect	CTVectSubtract(CTVect v1, CTVect v2) {
	CTVect rv = {
		.x = v1.x - v2.x,
		.y = v2.y - v2.y
	};
	return rv;
}

CTCALL	CTVect	CTVectMultiply(CTVect vect, FLOAT scalar) {
	CTVect rv = {
		.x = vect.x * scalar,
		.y = vect.y * scalar
	};
	return rv;
}

CTCALL	FLOAT	CTVectDot(CTVect v1, CTVect v2) {
	return (v1.x * v2.x + v1.y * v2.y);
}

CTCALL	CTVect	CTVectNormalize(CTVect vect) {

}

CTCALL	FLOAT	CTVectMagnitude(CTVect vect) {
	return sqrtf(vect.x * vect.x + vect.y * vect.y);
}

CTCALL	FLOAT	CTVectMagnitudeSqr(CTVect vect) {
	return vect.x * vect.x + vect.y * vect.y;
}

CTCALL	FLOAT	CTVectMagnitudeFast(CTVect vect) {

	// refer to <https://youtu.be/NWBEA2ECX-A> for details

	FLOAT min, max;

	if (vect.x > vect.y) {
		max = vect.x;
		min = vect.y;
	}
	else {
		max = vect.y;
		min = vect.x;
	}

	return max * 0.96f + min * 0.40f;
}

CTCALL	void	CTVectComponents(CTVect vect, PFLOAT x, PFLOAT y) {
	if (x != NULL) 
		*x = vect.x;
	if (y != NULL)
		*y = vect.y;
}

CTCALL	FLOAT	CTVectAngleDegrees(CTVect vect) {
	return atan2f(vect.y, vect.x) * CT_RADIANS_TO_DEGREES;
}