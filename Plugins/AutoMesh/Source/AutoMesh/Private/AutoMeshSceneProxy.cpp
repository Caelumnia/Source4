#include "AutoMeshSceneProxy.h"

#include "AutoMeshComponent.h"
#include "Stats.h"

FAutoMeshSceneProxy::FAutoMeshSceneProxy(UAutoMeshComponent* InComponent)
	: FStaticMeshSceneProxy(InComponent, false)
	, SectionBounds(InComponent->SectionBounds)
{
	FVector Location = InComponent->GetComponentLocation();
	for (auto& BS : SectionBounds)
	{
		BS.Origin += Location;
	}
}

bool FAutoMeshSceneProxy::CullSection(const FSceneView* View, int32 SectionIndex) const
{
	check(SectionIndex < SectionBounds.Num());
	
	const auto& BSBounds = SectionBounds[SectionIndex];
	if (View->ViewFrustum.IntersectSphere(BSBounds.Origin, BSBounds.SphereRadius))
	{
		if (View->ViewFrustum.IntersectBox(BSBounds.Origin, BSBounds.BoxExtent))
			return false;
	}
	return true;
}

bool FAutoMeshSceneProxy::GetMeshBatch(
	int32 LODIndex, int32 SectionIndex, int32 BatchIndex,
	uint8 DepthPriorityGroup, FMeshBatch& OutMeshBatch) const
{
	SCOPE_CYCLE_COUNTER(STAT_AutoMesh_GetMeshBatches);

	const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
	const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
	const FLODInfo& ProxyLODInfo = LODs[LODIndex];
	const FMaterialRenderProxy* MaterialRenderProxy = ProxyLODInfo.Sections[SectionIndex].Material->GetRenderProxy();

	FMeshBatchElement& OutMeshBatchElement = OutMeshBatch.Elements[BatchIndex];
	OutMeshBatch.Type = PT_TriangleList;
	OutMeshBatch.VertexFactory = &RenderData->LODVertexFactories[LODIndex].VertexFactory;
	OutMeshBatchElement.IndexBuffer = &LODModel.IndexBuffer;
	OutMeshBatchElement.FirstIndex = Section.FirstIndex;
	OutMeshBatchElement.NumPrimitives = Section.NumTriangles;

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

void FAutoMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	SCOPE_CYCLE_COUNTER(STAT_AutoMesh_DrawStaticMesh);
	checkSlow(IsInParallelRenderingThread());

	if (HasViewDependentDPG()) return;

	for (int32 LODIndex = 0; LODIndex < RenderData->LODResources.Num(); ++LODIndex)
	{
		FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
		float ScreenSize = RenderData->ScreenSize[LODIndex].GetValue();

		for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); ++SectionIndex)
		{
			const int32 NumBatches = 1;
			PDI->ReserveMemoryForMeshes(NumBatches);

			for (int32 BatchIndex = 0; BatchIndex < NumBatches; ++BatchIndex)
			{
				FMeshBatch MeshBatch;
				if (GetMeshBatch(LODIndex, SectionIndex, BatchIndex, GetStaticDepthPriorityGroup(), MeshBatch))
				{
					PDI->DrawMesh(MeshBatch, ScreenSize);
				}
			}
		}
	}
}

void FAutoMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
                                                 const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
                                                 FMeshElementCollector& Collector) const
{
	SCOPE_CYCLE_COUNTER(STAT_AutoMesh_GetDynamicMesh);
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

			FLODMask LODMask = GetLODMask(View);
			for (int32 LODIndex = 0; LODIndex < RenderData->LODResources.Num(); ++LODIndex)
			{
				if (LODIndex < ClampedMinLOD || !LODMask.ContainsLOD(LODIndex)) continue;
				
				FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
				if (EngineShowFlags.Wireframe && !EngineShowFlags.Materials && AllowDebugViewmodes())
				{
					
				}
				else
				{
					for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); ++SectionIndex)
					{
						if (SectionIndex < SectionBounds.Num() && CullSection(View, SectionIndex)) continue;
						
						const int32 NumBatches = 1;
						for (int32 BatchIndex = 0; BatchIndex < NumBatches; ++BatchIndex)
						{
							FMeshBatch& MeshBatch = Collector.AllocateMesh();

							if (GetMeshBatch(LODIndex, SectionIndex, BatchIndex, SDPG_World, MeshBatch))
							{
								MeshBatch.bDitheredLODTransition = false;
								MeshBatch.bCanApplyViewModeOverrides = true;
								MeshBatch.bUseWireframeSelectionColoring = false;

								Collector.AddMesh(ViewIndex, MeshBatch);
								INC_DWORD_STAT_BY(STAT_StaticMeshTriangles, MeshBatch.GetNumPrimitives());
								INC_DWORD_STAT_BY(STAT_AutoMesh_Triangles, MeshBatch.GetNumPrimitives());
							}
						}
					}
				}
			}
			INC_DWORD_STAT_BY(STAT_AutoMesh_Batches, Collector.GetMeshBatchCount(ViewIndex));
		}
	}
}

FPrimitiveViewRelevance FAutoMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	checkSlow(IsInParallelRenderingThread())

	FPrimitiveViewRelevance Relevance;
	Relevance.bStaticRelevance = false;
	Relevance.bDynamicRelevance = true;
	Relevance.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.StaticMeshes;
	Relevance.bShadowRelevance = IsShadowCast(View);
	Relevance.bRenderCustomDepth = ShouldRenderCustomDepth();
	Relevance.bRenderInDepthPass = false;
	Relevance.bRenderInMainPass = ShouldRenderInMainPass();
	Relevance.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Relevance.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	MaterialRelevance.SetPrimitiveViewRelevance(Relevance);
	Relevance.bOpaque = true;
	Relevance.bVelocityRelevance = IsMovable() && Relevance.bOpaque && Relevance.bRenderInMainPass;

	return Relevance;
}
