//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_color.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"

static __forceinline BYTE __HCTClampColorChannel(BYTE c) {
	if (c < 0)
		return 0;
	if (c > 255)
		return 255;
	return c;
}

CTCALL	CTColor			CTColorCreate(INT r, INT g, INT b, INT a) {
	CTColor rc = {
		.r = __HCTClampColorChannel(r),
		.g = __HCTClampColorChannel(g),
		.b = __HCTClampColorChannel(b),
		.a = __HCTClampColorChannel(a)
	};
	return rc;
}

CTCALL	CTColor			CTColorScale(CTColor col, FLOAT factor) {
	CTColor rc = {
		.r = __HCTClampColorChannel((FLOAT)col.r * factor),
		.g = __HCTClampColorChannel((FLOAT)col.g * factor),
		.b = __HCTClampColorChannel((FLOAT)col.b * factor),
		.a = __HCTClampColorChannel((FLOAT)col.a * factor)
	};
	return rc;
}

CTCALL	CTColor			CTColorMultiply(CTColor c1, CTColor c2) {
	CTColor rc = {
		.r = __HCTClampColorChannel((c1.r * c2.r) >> 8),
		.g = __HCTClampColorChannel((c1.g * c2.g) >> 8),
		.b = __HCTClampColorChannel((c1.b * c2.b) >> 8),
		.a = __HCTClampColorChannel((c1.a * c2.a) >> 8)
	};
	return rc;
}

CTCALL	CTColor			CTColorAdd(CTColor c1, CTColor c2) {
	CTColor rc = {
		.r = __HCTClampColorChannel(c1.r + c2.r),
		.g = __HCTClampColorChannel(c1.g + c2.g),
		.b = __HCTClampColorChannel(c1.b + c2.b),
		.a = __HCTClampColorChannel(c1.a + c2.a)
	};
	return rc;
}

CTCALL	CTColor			CTColorBlend(CTColor bottom, CTColor top) {

	if (top.a == 255) {
		return top;
	}

	if (top.a == 0) {
		return bottom;
	}
	
	CTColor rc = {
		.r = __HCTClampColorChannel((((top.r - bottom.r) * top.a) >> 8) + bottom.r),
		.g = __HCTClampColorChannel((((top.g - bottom.g) * top.a) >> 8) + bottom.g),
		.b = __HCTClampColorChannel((((top.b - bottom.b) * top.a) >> 8) + bottom.b),
		.a = __HCTClampColorChannel(255)
	};
	return rc;
}

CTCALL	CTColor			CTColorBlendWeighted(CTColor c1, CTColor c2, FLOAT factor) {
	c2.a = __HCTClampColorChannel((255.0f * factor));
	return CTColorBlend(c1, c2);
}
