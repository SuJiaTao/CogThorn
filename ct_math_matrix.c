//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_math_matrix.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_math.h"

#include <math.h>
#include <intrin.h>

static void __HCTMatrixInit(PCTMatrix pMat) {
	__stosb(pMat, 0, sizeof(*pMat));
}

static void __HCTMatrixSet(PCTMatrix pMat, FLOAT val, INT row, INT col) {
	pMat->vals[row][col] = val;
}

CTCALL	CTMatrix	CTMatrixIdentity() {
	CTMatrix rMat;

	__HCTMatrixInit(&rMat);

	__HCTMatrixSet(&rMat, 1, 0, 0);
	__HCTMatrixSet(&rMat, 1, 1, 1);
	__HCTMatrixSet(&rMat, 1, 2, 2);

	return rMat;
}

CTCALL	CTMatrix	CTMatrixMultiply(CTMatrix m1, CTMatrix m2) {
	CTMatrix rMat;

	__HCTMatrixInit(&rMat);

	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {

			FLOAT fa [4] = { m2.vals[0][y], m2.vals[1][y], m2.vals[2][y], 0.0f };
			FLOAT fb [4] = { m1.vals[x][0], m1.vals[x][1], m1.vals[x][2], 0.0f };

			__m128 va, vb, vr, sum;
			va  = _mm_loadu_ps(fa);
			vb  = _mm_loadu_ps(fb);
			vr  = _mm_mul_ps(va, vb);
			sum = _mm_hadd_ps(vr, vr);
			sum = _mm_hadd_ps(sum, sum);
			_mm_store_ss(&rMat.vals[x][y], sum);

		}
	}

	return rMat;
}

CTCALL	CTMatrix	CTMatrixScale(CTMatrix mat, CTVect scl) {

	if (scl.x == 1.0f && scl.y == 1.0f)
		return mat;

	CTMatrix mulMat = CTMatrixIdentity();

	__HCTMatrixSet(&mulMat, scl.x, 0, 0);
	__HCTMatrixSet(&mulMat, scl.y, 1, 1);

	return CTMatrixMultiply(mat, mulMat);
}

CTCALL	CTMatrix	CTMatrixTranslate(CTMatrix mat, CTVect trl) {

	if (trl.x == 0.0f && trl.y == 0.0f)
		return mat;

	CTMatrix mulMat = CTMatrixIdentity();

	__HCTMatrixSet(&mulMat, trl.x, 2, 0);
	__HCTMatrixSet(&mulMat, trl.y, 2, 1);

	return CTMatrixMultiply(mat, mulMat);
}

CTCALL	CTMatrix	CTMatrixRotate(CTMatrix mat, FLOAT rotation) {

	if (rotation == 0.0f) 
		return mat;

	CTMatrix mulMat = CTMatrixIdentity();

	FLOAT cosine	= cosf(rotation * CT_DEGREES_TO_RADIANS);
	FLOAT sine		= sinf(rotation * CT_DEGREES_TO_RADIANS);

	__HCTMatrixSet(&mulMat, cosine, 0, 0);
	__HCTMatrixSet(&mulMat, -sine,  1, 0);
	__HCTMatrixSet(&mulMat, sine,   0, 1);
	__HCTMatrixSet(&mulMat, cosine, 1, 1);

	return CTMatrixMultiply(mat, mulMat);
}

CTCALL	CTVect		CTMatrixApply(CTMatrix mat, CTVect vect) {
	FLOAT vect3[] = {
		vect.x,
		vect.y,
		1.0f
	};

	FLOAT vec3Rslt[] = {
		0.0f,
		0.0f,
		0.0f
	};

	for (int y = 0; y < 3; y++) {
		vec3Rslt[y] = 
			mat.vals[0][y] * vect3[0] + 
			mat.vals[1][y] * vect3[1] + 
			mat.vals[2][y] * vect3[2];
	}

	CTVect rVect = {
		.x = vec3Rslt[0],
		.y = vec3Rslt[1]
	};

	return rVect;
}

CTCALL	CTMatrix	CTMatrixTransform(CTMatrix mat, CTVect trl, CTVect scl, FLOAT rot) {
	return CTMatrixTranslate(
		CTMatrixRotate(
			CTMatrixScale(
				mat,
				scl
			),
			rot
		),
		trl
	);
}
