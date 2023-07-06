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
	rs->pointSizePixels			= max(CT_SHADER_POINTSIZE_MIN, min(pointSize, CT_SHADER_POINTSIZE_MAX));
	rs->lineSizePixels			= max(CT_SHADER_LINESIZE_MIN,  min(lineSize,  CT_SHADER_LINESIZE_MAX));
	rs->depthTest				= depthTest;
	return rs;
}

CTCALL	BOOL		CTShaderDestroy(PCTShader* pShader) {
	if (pShader == NULL) {
		CTErrorSetBadObject("CTShader destroy failed: pShader was NULL");
		return FALSE;
	}

	PCTShader shader = *pShader;
	if (shader == NULL) {
		CTErrorSetBadObject("CTShader destroy failed: shader was NULL");
		return FALSE;
	}

	CTGFXFree(shader);
	*pShader = NULL;
	return TRUE;
}
