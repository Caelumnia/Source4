#pragma once

DECLARE_STATS_GROUP(TEXT("GigaMesh"), STATGROUP_GigaMesh, STATCAT_Advanced)
DECLARE_DWORD_COUNTER_STAT(TEXT("GigaMesh Triangles"), STAT_GigaMesh_Triangles, STATGROUP_GigaMesh)
DECLARE_DWORD_COUNTER_STAT(TEXT("GigaMesh Batches"), STAT_GigaMesh_Batches, STATGROUP_GigaMesh)
DECLARE_CYCLE_STAT(TEXT("GetDynamicMesh"), STAT_GigaMesh_GetDynamicMeshBatch, STATGROUP_GigaMesh)
DECLARE_CYCLE_STAT(TEXT("GetDynamicMesh"), STAT_GigaMesh_DrawStaticMeshBatch, STATGROUP_GigaMesh)
DECLARE_CYCLE_STAT(TEXT("GetMeshBatches"), STAT_GigaMesh_GetMeshBatches, STATGROUP_GigaMesh)
DECLARE_CYCLE_STAT(TEXT("UpdateVisibility"), STAT_GigaMesh_UpdateVisibility, STATGROUP_GigaMesh)