#pragma once

DECLARE_STATS_GROUP(TEXT("AutoMesh"), STATGROUP_AutoMesh, STATCAT_Advanced)
DECLARE_DWORD_COUNTER_STAT(TEXT("AutoMesh Triangles"), STAT_AutoMesh_Triangles, STATGROUP_AutoMesh)
DECLARE_DWORD_COUNTER_STAT(TEXT("AutoMesh Batches"), STAT_AutoMesh_Batches, STATGROUP_AutoMesh)
DECLARE_CYCLE_STAT(TEXT("DrawStaticMesh"), STAT_AutoMesh_DrawStaticMesh, STATGROUP_AutoMesh)
DECLARE_CYCLE_STAT(TEXT("GetDynamicMesh"), STAT_AutoMesh_GetDynamicMesh, STATGROUP_AutoMesh)
DECLARE_CYCLE_STAT(TEXT("GetMeshBatches"), STAT_AutoMesh_GetMeshBatches, STATGROUP_AutoMesh)