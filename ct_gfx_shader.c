//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_shader.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"

CTCALL	PCTShader	CTShaderCreate(PCTSPRIMITIVE sPrim, PCTSPIXEL sPix, SIZE_T shaderInputSize) {
	PCTShader rs				= CTGFXAlloc(sizeof(*rs));
	rs->primitiveShader			= sPrim;
	rs->pixelShader				= sPix;
	rs->shaderInputSizeBytes	= shaderInputSize;
	return rs;
}

CTCALL	BOOL		CTShaderDestroy(PCTShader shader) {
	CTGFXFree(shader);
	return TRUE;
}
