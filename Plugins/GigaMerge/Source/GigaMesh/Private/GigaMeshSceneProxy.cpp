#include "GigaMeshSceneProxy.h"

#include "GigaMeshComponent.h"
#include "GigaStats.h"

FGigaMeshSceneProxy::FGigaMeshSceneProxy(UGigaMeshComponent* InComponent, UGigaMesh* InMesh) :
	FStaticMeshSceneProxy(InComponent, false)
{
	FTransform LocalTransforms = InComponent->GetComponentTransform();
	auto& BatchMap = InMesh->BatchMap;
	for (int32 LODIndex = 0; LODIndex < RenderData->LODResources.Num(); ++LODIndex)
	{
		FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
		LODModel.IndexBuffer.TrySetAllowCPUAccess(true);
		TArray<uint32> Indices;
		LODModel.IndexBuffer.GetCopy(Indices);
		LODModel.IndexBuffer.TrySetAllowCPUAccess(false);

		for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); ++SectionIndex)
		{
			const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
			const uint64 BatchIndex = GetCombinedBatchIndex(LODIndex, SectionIndex);
			FGigaBatch Batch = BatchMap.GetBatch(LODIndex, SectionIndex);
			for (auto& Element : Batch.Elements)
			{
				Element.Bounds = Element.Bounds.TransformBy(LocalTransforms);
			}

			FGigaIndexBuffer IndexBuffer{Indices, MoveTemp(Batch), Section.FirstIndex, Section.NumTriangles};
			DynamicIndices.Add(BatchIndex, MoveTemp(IndexBuffer));
		}
	}
}

bool FGigaMeshSceneProxy::GetMeshBatch(const FSceneView* View, int32 LODIndex, int32 SectionIndex, uint8 DepthPriorityGroup, FMeshBatch& OutMeshBatch) const
{
	SCOPE_CYCLE_COUNTER(STAT_GigaMesh_GetMeshBatches);

	const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
	const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
	const FLODInfo& ProxyLODInfo = LODs[LODIndex];
	const FMaterialRenderProxy* MaterialRenderProxy = ProxyLODInfo.Sections[SectionIndex].Material->GetRenderProxy();

	const uint64 BatchIndex = GetCombinedBatchIndex(LODIndex, SectionIndex);
	check(DynamicIndices.Contains(BatchIndex))
	FGigaIndexBuffer& IndexBuffer = DynamicIndices[BatchIndex];
	IndexBuffer.UpdateVisibility(View->ViewFrustum);

	FMeshBatchElement& OutMeshBatchElement = OutMeshBatch.Elements[0];
	OutMeshBatch.Type = PT_TriangleList;
	OutMeshBatch.VertexFactory = &RenderData->LODVertexFactories[LODIndex].VertexFactory;
	OutMeshBatchElement.IndexBuffer = &IndexBuffer;
	OutMeshBatchElement.FirstIndex = 0;
	OutMeshBatchElement.NumPrimitives = IndexBuffer.GetNumTriangles();

	if (OutMeshBatchElement.NumPrimitives > 0)
	{
		OutMeshBatch.SegmentIndex = SectionIndex;
		OutMeshBatch.LODIndex = LODIndex;
		OutMeshBatch.CastShadow = bCastShadow && Section.bCastShadow;
		OutMeshBatch.DepthPriorityGroup = DepthPriorityGroup;
		OutMeshBatch.LCI = nullptr;
		OutMeshBatch.MaterialRenderProxy = MaterialRenderProxy;

		OutMeshBatchElement.MinVertexIndex = Section.MinVertexIndex;
		OutMeshBatchElement.MaxVertexIndex = Section.MaxVertexIndex;
		OutMeshBatchElement.MinScreenSize = LODIndex < MAX_STATIC_MESH_LODS - 1
			                                    ? RenderData->ScreenSize[LODIndex].GetValue()
			                                    : 0.0f;
		OutMeshBatchElement.MaxScreenSize = RenderData->ScreenSize[LODIndex + 1].GetValue();

		return true;
	}

	return false;
}

void FGigaMeshSceneProxy::CreateRenderThreadResources()
{
	FStaticMeshSceneProxy::CreateRenderThreadResources();
	for (auto& Tuple : DynamicIndices)
	{
		auto& Buffer = Tuple.Value;
		Buffer.InitResource();
	}
}

void FGigaMeshSceneProxy::DestroyRenderThreadResources()
{
	FStaticMeshSceneProxy::DestroyRenderThreadResources();
	for (auto& Tuple : DynamicIndices)
	{
		auto& Buffer = Tuple.Value;
		Buffer.ReleaseResource();
	}
}

void FGigaMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
                                                 FMeshElementCollector& Collector) const
{
	SCOPE_CYCLE_COUNTER(STAT_GigaMesh_GetDynamicMesh);
	checkSlow(IsInParallelRenderingThread());

	const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;
	bool bDrawSimpleCollision = false, bDrawComplexCollision = false;
	const bool bInCollisionView = IsCollisionView(EngineShowFlags, bDrawSimpleCollision, bDrawComplexCollision);
	const bool bForceDynamicDraw = IsRichView(ViewFamily) || HasViewDependentDPG();
	const bool bFocus = IsSelected() || IsHovered();
	const bool bLightmapSettingError = HasStaticLighting() && !HasValidSettingsForStaticLighting();

	const bool bDrawMesh = !bInCollisionView && (bForceDynamicDraw || bFocus || bLightmapSettingError || EngineShowFlags.Collision || EngineShowFlags.Bounds);

	if (EngineShowFlags.StaticMeshes /*&& bDrawMesh*/)
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
		{
			const FSceneView* View = Views[ViewIndex];
			if (!(VisibilityMap & (1 << ViewIndex)) || !IsShown(View)) continue;

			// NOTE THAT Dithered LOD Transition would not work.
			const int32 LODIndex = GetLOD(View);
			if (LODIndex < ClampedMinLOD) continue;

			FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
			if (EngineShowFlags.Wireframe && !EngineShowFlags.Materials && AllowDebugViewmodes()) { }
			else
			{
				for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); ++SectionIndex)
				{
					FMeshBatch& MeshBatch = Collector.AllocateMesh();

					if (GetMeshBatch(View, LODIndex, SectionIndex, SDPG_World, MeshBatch))
					{
						MeshBatch.bDitheredLODTransition = false;
						MeshBatch.bCanApplyViewModeOverrides = true;
						MeshBatch.bUseWireframeSelectionColoring = false;

						Collector.AddMesh(ViewIndex, MeshBatch);
						INC_DWORD_STAT_BY(STAT_StaticMeshTriangles, MeshBatch.GetNumPrimitives());
						INC_DWORD_STAT_BY(STAT_GigaMesh_Triangles, MeshBatch.GetNumPrimitives());
					}
				}
			}

			INC_DWORD_STAT_BY(STAT_GigaMesh_Batches, Collector.GetMeshBatchCount(ViewIndex));
		}
	}
}

FPrimitiveViewRelevance FGigaMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Relevance = FStaticMeshSceneProxy::GetViewRelevance(View);
	Relevance.bStaticRelevance = false;
	Relevance.bDynamicRelevance = true;

	return Relevance;
}
