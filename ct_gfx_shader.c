//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_shader.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"

CTCALL	PCTShader	CTShaderCreate(
	PCTSPRIMITIVE	sPrim,
	PCTSPIXEL		sPix,
	SIZE_T			shaderInputSize,
	UINT32			pointSize,
	UINT32			lineSize,
	BOOL			depthTest
) {
	PCTShader rs				= CTGFXAlloc(sizeof(*rs));
	rs->primitiveShader			= sPrim;
	rs->pixelShader				= sPix;
	rs->shaderInputSizeBytes	= shaderInputSize;
	rs->pointSizePixels			= pointSize;
	rs->lineSizePixels			= lineSize;
	rs->depthTest				= depthTest;
	return rs;
}

CTCALL	BOOL		CTShaderDestroy(PCTShader shader) {
	CTGFXFree(shader);
	return TRUE;
}
