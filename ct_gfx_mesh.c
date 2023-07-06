//////////////////////////////////////////////////////////////////////////////
///	
/// 							<ct_gfx_mesh.c>
///								Bailey JT Brown
///								2023
/// 
//////////////////////////////////////////////////////////////////////////////

#include "ct_gfx.h"
#include <intrin.h>

CTCALL	PCTMesh		CTMeshCreate(PFLOAT verts, PFLOAT uvs, UINT32 primCount) {
	if (verts == NULL) {
		CTErrorSetParamValue("CTMeshCreated failed: verts was NULL");
		return NULL;
	}
	if (uvs == NULL) {
		CTErrorSetParamValue("CTMeshCreated failed: uvs was NULL");
		return NULL;
	}
	if (primCount == 0) {
		CTErrorSetParamValue("CTMeshCreated failed: primCount was 0");
		return NULL;
	}

	PCTMesh rMesh		= CTGFXAlloc(sizeof(*rMesh));
	rMesh->primList		= CTGFXAlloc(sizeof(*rMesh->primList) * primCount);
	rMesh->primCount	= primCount;

	for (UINT32 primID = 0; primID < primCount; primID++) {

		UINT32 compXIndex = (primID * 2) + 0;
		UINT32 compYIndex = (primID * 2) + 1;

		rMesh->primList[primID].vertex.x	= verts[compXIndex];
		rMesh->primList[primID].vertex.y	= verts[compYIndex];
		rMesh->primList[primID].UV.x		= uvs[compXIndex];
		rMesh->primList[primID].UV.y		= uvs[compYIndex];

	}

	return rMesh;
}

CTCALL	BOOL		CTMeshDestroy(PCTMesh* pMesh) {
	if (pMesh == NULL) {
		CTErrorSetBadObject("CTMeshDestroy failed: pMesh was NULL");
		return FALSE;
	}

	PCTMesh mesh = *pMesh;

	if (mesh == NULL) {
		CTErrorSetBadObject("CTMeshDestroy failed: mesh was NULL");
		return FALSE;
	}

	CTGFXFree(mesh->primList);
	CTGFXFree(mesh);

	*pMesh = NULL;
	return TRUE;
}