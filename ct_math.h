//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_math.h>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#ifndef _CT_MATH_INCLUDE_
#define _CT_MATH_INCLUDE_

#include "ct_base.h"

//////////////////////////////////////////////////////////////////////////////
///
///								2D VECTOR OPERATIONS
/// 
//////////////////////////////////////////////////////////////////////////////

#define CT_RADIANS_TO_DEGREES	57.2958f
#define CT_DEGREES_TO_RADIANS	0.0174533f

typedef struct CTVect {
	FLOAT x, y;
} CTVect, *PCTVect;

CTCALL	CTVect	CTVectCreate(FLOAT x, FLOAT y);
CTCALL	CTVect	CTVectAdd(CTVect v1, CTVect v2);
CTCALL	CTVect	CTVectSubtract(CTVect v1, CTVect v2);
CTCALL	CTVect	CTVectMultiply(CTVect vect, FLOAT scalar);
CTCALL	FLOAT	CTVectDot(CTVect v1, CTVect v2);
CTCALL	CTVect	CTVectNormalize(CTVect vect);
CTCALL	FLOAT	CTVectMagnitude(CTVect vect);
CTCALL	FLOAT	CTVectMagnitudeSqr(CTVect vect);
CTCALL	FLOAT	CTVectMagnitudeFast(CTVect vect);
CTCALL	void	CTVectComponents(CTVect vect, PFLOAT x, PFLOAT y);
CTCALL	FLOAT	CTVectAngleDegrees(CTVect vect);

//////////////////////////////////////////////////////////////////////////////
///
///							3x3 AFFINE TRANSFORM MATRIX
/// 
//////////////////////////////////////////////////////////////////////////////

typedef struct CTMatrix {
	FLOAT vals[3][3];
} CTMatrix, *PCTMatrix;

CTCALL	CTMatrix	CTMatrixIdentity();
CTCALL	CTMatrix	CTMatrixMultiply(CTMatrix m1, CTMatrix m2);
CTCALL	CTMatrix	CTMatrixScale(CTMatrix mat, CTVect scl);
CTCALL	CTMatrix	CTMatrixTranslate(CTMatrix mat, CTVect trl);
CTCALL	CTMatrix	CTMatrixRotate(CTMatrix mat, FLOAT rotation);
CTCALL	CTVect		CTMatrixApply(CTMatrix mat, CTVect vect);
CTCALL	CTMatrix	CTMatrixTransform(CTMatrix mat, CTVect trl, CTVect scl, FLOAT rot);

//////////////////////////////////////////////////////////////////////////////
///
///								RANDOM
/// 
//////////////////////////////////////////////////////////////////////////////

CTCALL	UINT32		CTRandomSeed(void);
CTCALL	UINT32		CTRandomInt(UINT32 seed);
CTCALL	FLOAT		CTRandomFloat(FLOAT seed);
CTCALL	BOOL		CTRandomChance(UINT32 seed, UINT32 num, UINT32 denom);

#endif
