#include "AutoMergeHelper.h"

#include "MaterialUtilities.h"
#include "MeshMergeDataTracker.h"
#include "StaticMeshOperations.h"

UStaticMesh* AutoMergeHelper::MergeComponents(UObject* Outer, const FMeshComponentArray& MeshComponents, FVector Pivot)
{
	FMeshMergeDataTracker DataTracker;
	TArray<FSectionInfo> SectionInfos;

	// Trace all static meshes
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(RetrieveRawMeshData);
		for (int32 ComponentIndex = 0; ComponentIndex < MeshComponents.Num(); ++ComponentIndex)
		{
			auto Component = MeshComponents[ComponentIndex];
			int32 LightMapWidth, LightMapHeight;
			if (Component->GetLightMapResolution(LightMapWidth, LightMapHeight))
			{
				DataTracker.AddLightMapPixels(LightMapWidth * LightMapHeight);
			}

			const int32 NumLODs = Component->GetStaticMesh()->GetNumLODs();
			const FTransform ComponentToWorld = Component->GetComponentTransform();
			for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
			{
				FMeshDescription& RawMesh = DataTracker.AddAndRetrieveRawMesh(
					ComponentIndex, LODIndex, Component->GetStaticMesh());

				const UStaticMesh* StaticMesh = Component->GetStaticMesh();
				// Retrieve raw mesh data
				{
					const FStaticMeshSourceModel& SrcModel = StaticMesh->GetSourceModel(LODIndex);
					const bool bIsImported = StaticMesh->IsMeshDescriptionValid(LODIndex);
					const FMeshBuildSettings BuildSettings = bIsImported
						                                         ? SrcModel.BuildSettings
						                                         : StaticMesh->GetSourceModel(0).BuildSettings;

					ExportStaticMeshLOD(StaticMesh->GetRenderData()->LODResources[LODIndex],
					                    StaticMesh->GetStaticMaterials(), RawMesh);
					FStaticMeshOperations::ApplyTransform(RawMesh, ComponentToWorld);
					EComputeNTBsFlags ComputeNtBsFlags = EComputeNTBsFlags::BlendOverlappingNormals;
					if (BuildSettings.bRemoveDegenerates)
					{
						ComputeNtBsFlags |= EComputeNTBsFlags::IgnoreDegenerateTriangles;
					}
					if (BuildSettings.bUseMikkTSpace)
					{
						ComputeNtBsFlags |= EComputeNTBsFlags::UseMikkTSpace;
					}
					FStaticMeshOperations::ComputePolygonTangentsAndNormals(RawMesh);
					FStaticMeshOperations::RecomputeNormalsAndTangentsIfNeeded(RawMesh, ComputeNtBsFlags);
				}

				SectionInfos.SetNum(0, false);
				ExtractSections(Component, LODIndex, SectionInfos);

				for (int32 SectionIndex = 0; SectionIndex < SectionInfos.Num(); ++SectionIndex)
				{
					const FSectionInfo& SectionInfo = SectionInfos[SectionIndex];
					const int32 UniqueIndex = DataTracker.AddSection(SectionInfo);

					DataTracker.AddSectionRemapping(ComponentIndex, LODIndex, SectionIndex, UniqueIndex);
					DataTracker.AddMaterialSlotName(SectionInfo.Material, SectionInfo.MaterialSlotName);

					FStaticMeshOperations::SwapPolygonPolygonGroup(RawMesh, UniqueIndex, SectionInfo.StartIndex,
					                                               SectionInfo.EndIndex, false);
				}

				FElementIDRemappings RemapInfo;
				RawMesh.Compact(RemapInfo);

				if (RawMesh.VertexInstances().Num() > 0)
				{
					if (StaticMesh)
					{
						DataTracker.AddLightmapChannelRecord(ComponentIndex, LODIndex,
						                                     StaticMesh->GetLightMapCoordinateIndex());
					}
					DataTracker.AddLODIndex(LODIndex);
				}
			}
		}
	}
	DataTracker.ProcessRawMeshes();

	// Find all unique materials and remap section
	const int32 NumSections = DataTracker.NumberOfUniqueSections();
	TArray<UMaterialInterface*> UniqueMaterials;
	UniqueMaterials.AddDefaulted(NumSections);
	for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
	{
		if (!UniqueMaterials[SectionIndex])
			UniqueMaterials[SectionIndex] = DataTracker.GetMaterialForSectionIndex(SectionIndex);
	}
	TArray<float> MaterialImportance;
	FMaterialUtilities::DetermineMaterialImportance(UniqueMaterials, MaterialImportance);

	// Create merged meshes
	const int32 NumLODs = DataTracker.GetNumLODsForMergedMesh();
	TArray<FMeshDescription> MergedRawMeshes;
	MergedRawMeshes.AddDefaulted(NumLODs);
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		FMeshDescription& MergedMesh = MergedRawMeshes[LODIndex];
		FStaticMeshAttributes(MergedMesh).Register();

		for (int32 ComponentIndex = 0; ComponentIndex < MeshComponents.Num(); ++ComponentIndex)
		{
			int32 RetrievedLODIndex = LODIndex;
			FMeshDescription* RawMeshPtr = DataTracker.TryFindRawMeshForLOD(ComponentIndex, RetrievedLODIndex);

			if (!RawMeshPtr) continue;
			DataTracker.AddComponentToWedgeMapping(ComponentIndex, LODIndex, MergedMesh.VertexInstances().Num());

			FStaticMeshOperations::FAppendSettings AppendSettings;
			AppendSettings.bMergeVertexColor = false;
			AppendSettings.MergedAssetPivot = Pivot;
			for (int32 ChannelIndex = 0; ChannelIndex < FStaticMeshOperations::FAppendSettings::MAX_NUM_UV_CHANNELS; ++
			     ChannelIndex)
			{
				AppendSettings.bMergeUVChannels[ChannelIndex] = DataTracker.DoesUVChannelContainData(
					ChannelIndex, LODIndex);
			}
			AppendSettings.PolygonGroupsDelegate = FAppendPolygonGroupsDelegate::CreateLambda(
				[&](const FMeshDescription& SourceMesh, FMeshDescription& TargetMesh,
				    PolygonGroupMap& RemapPolygonGroups)
				{
					TPolygonGroupAttributesConstRef<FName> SourceImportedMatSlotNames = SourceMesh.
						PolygonGroupAttributes().GetAttributesRef<FName>(
							MeshAttribute::PolygonGroup::ImportedMaterialSlotName);
					TPolygonGroupAttributesRef<FName> TargetImportedMatSlotNames = TargetMesh.PolygonGroupAttributes().
						GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

					TArray<SectionRemapPair> SectionMappings;
					DataTracker.GetMappingsForMeshLOD(FMeshLODKey{ComponentIndex, LODIndex}, SectionMappings);
					for (FPolygonGroupID SourceID : SourceMesh.PolygonGroups().GetElementIDs())
					{
						int32 UniqueIndex = SourceID.GetValue();
						FPolygonGroupID TargetID{UniqueIndex};
						if (!TargetMesh.PolygonGroups().IsValid(TargetID))
						{
							while (TargetMesh.PolygonGroups().Num() <= UniqueIndex)
							{
								TargetID = TargetMesh.CreatePolygonGroup();
							}
							check(TargetID.GetValue() == UniqueIndex);
							TargetImportedMatSlotNames[TargetID] = SourceImportedMatSlotNames[SourceID];
						}
						RemapPolygonGroups.Add(SourceID, TargetID);
					}
				});
			FStaticMeshOperations::AppendMeshDescription(*RawMeshPtr, MergedMesh, AppendSettings);
		}

		TArray<FPolygonGroupID> PolygonGroupIdToRemove;
		for (auto ID : MergedMesh.PolygonGroups().GetElementIDs())
		{
			if (MergedMesh.GetPolygonGroupPolygons(ID).Num() < 1)
			{
				PolygonGroupIdToRemove.Add(ID);
			}
		}
		for (auto ID : PolygonGroupIdToRemove)
		{
			MergedMesh.DeletePolygonGroup(ID);
		}
	}

	// Populate mesh section map
	FMeshSectionInfoMap SectionInfoMap;
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		const auto& TargetRawMesh = MergedRawMeshes[LODIndex];
		TArray<uint32> UniqueMatIndices;
		uint32 MaterialIndex = 0;
		for (auto ID : TargetRawMesh.PolygonGroups().GetElementIDs())
		{
			if (TargetRawMesh.GetNumPolygonGroupPolygons(ID) == 0) continue;

			if (ID.GetValue() < DataTracker.NumberOfUniqueSections())
			{
				UniqueMatIndices.AddUnique(ID.GetValue());
			}
			else
			{
				UniqueMatIndices.AddUnique(MaterialIndex);
			}
			MaterialIndex++;
		}
		UniqueMatIndices.Sort();
		for (int32 Index = 0; Index < UniqueMatIndices.Num(); ++Index)
		{
			const int32 SectionIndex = UniqueMatIndices[Index];
			const FSectionInfo& SectionInfo = DataTracker.GetSection(SectionIndex);
			FMeshSectionInfo MeshSectionInfo;
			MeshSectionInfo.bCastShadow = SectionInfo.EnabledProperties.Contains(
				GET_MEMBER_NAME_CHECKED(FMeshSectionInfo, bCastShadow));
			MeshSectionInfo.bEnableCollision = SectionInfo.EnabledProperties.Contains(
				GET_MEMBER_NAME_CHECKED(FMeshSectionInfo, bEnableCollision));
			MeshSectionInfo.MaterialIndex = UniqueMaterials.IndexOfByKey(SectionInfo.Material);
			SectionInfoMap.Set(LODIndex, Index, MeshSectionInfo);
		}
	}

	// Create merged mesh asset
	UStaticMesh* OutMergedMesh = nullptr;
	{
		OutMergedMesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Dynamic | RF_Transient);
		OutMergedMesh->InitResources();

		const int32 LightMapUVChannel = DataTracker.GetAvailableLightMapUVChannel();
		if (LightMapUVChannel == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to find an available channel for Lightmap UVs."));
		}
		else
		{
			OutMergedMesh->SetLightingGuid();
			OutMergedMesh->SetLightMapResolution(256);
			OutMergedMesh->SetLightMapCoordinateIndex(LightMapUVChannel);
		}

		for (int32 LODIndex = 0; LODIndex < MergedRawMeshes.Num(); ++LODIndex)
		{
			FMeshDescription& MergedRawMesh = MergedRawMeshes[LODIndex];
			if (MergedRawMesh.Vertices().Num() <= 0) continue;

			FStaticMeshSourceModel& SrcModel = OutMergedMesh->AddSourceModel();
			SrcModel.BuildSettings.bRecomputeNormals = false;
			SrcModel.BuildSettings.bRecomputeTangents = false;
			SrcModel.BuildSettings.bRemoveDegenerates = false;
			SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
			SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
			SrcModel.BuildSettings.bGenerateLightmapUVs = LightMapUVChannel != INDEX_NONE;
			SrcModel.BuildSettings.MinLightmapResolution = 256;
			SrcModel.BuildSettings.SrcLightmapIndex = 0;
			SrcModel.BuildSettings.DstLightmapIndex = LightMapUVChannel != INDEX_NONE ? LightMapUVChannel : 0;
			SrcModel.BuildSettings.DistanceFieldResolutionScale = 0.0f;

			OutMergedMesh->CreateMeshDescription(LODIndex, MergedRawMesh);
			OutMergedMesh->CommitMeshDescription(LODIndex);
		}

		for (UMaterialInterface* Material : UniqueMaterials)
		{
			if (Material && !Material->IsAsset())
			{
				Material = nullptr;
			}
			FName MaterialSlotName = DataTracker.GetMaterialSlotName(Material);
			OutMergedMesh->GetStaticMaterials().Add(FStaticMaterial{Material, MaterialSlotName});
		}

		OutMergedMesh->GetSectionInfoMap().CopyFrom(SectionInfoMap);
		OutMergedMesh->GetOriginalSectionInfoMap().CopyFrom(SectionInfoMap);

		OutMergedMesh->Build();
		OutMergedMesh->PostEditChange();
	}
	return OutMergedMesh;
}

void AutoMergeHelper::ExportStaticMeshLOD(
	const FStaticMeshLODResources& StaticMeshLOD,
	const TArray<FStaticMaterial>& Materials,
	FMeshDescription& OutRawMesh
)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ExportStaticMeshLOD);
	const int32 NumWedges = StaticMeshLOD.IndexBuffer.GetNumIndices();
	const int32 NumVertexPositions = StaticMeshLOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();
	const int32 NumFaces = NumWedges / 3;

	OutRawMesh.Empty();

	if (NumVertexPositions <= 0 || StaticMeshLOD.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices() <= 0)
	{
		return;
	}

	TVertexAttributesRef<FVector> VertexPositions = OutRawMesh.VertexAttributes().GetAttributesRef<FVector>(
		MeshAttribute::Vertex::Position);
	TEdgeAttributesRef<bool> EdgeHardnesses = OutRawMesh.EdgeAttributes().GetAttributesRef<bool>(
		MeshAttribute::Edge::IsHard);
	TEdgeAttributesRef<float> EdgeCreaseSharpnesses = OutRawMesh.EdgeAttributes().GetAttributesRef<float>(
		MeshAttribute::Edge::CreaseSharpness);
	TPolygonGroupAttributesRef<FName> PolygonGroupImportedMaterialSlotNames = OutRawMesh.PolygonGroupAttributes().
		GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);
	TVertexInstanceAttributesRef<FVector> VertexInstanceNormals = OutRawMesh.VertexInstanceAttributes().GetAttributesRef
		<FVector>(MeshAttribute::VertexInstance::Normal);
	TVertexInstanceAttributesRef<FVector> VertexInstanceTangents = OutRawMesh.VertexInstanceAttributes().
	                                                                          GetAttributesRef<FVector>(
		                                                                          MeshAttribute::VertexInstance::Tangent);
	TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = OutRawMesh.VertexInstanceAttributes().
		GetAttributesRef<float>(MeshAttribute::VertexInstance::BinormalSign);
	TVertexInstanceAttributesRef<FVector4> VertexInstanceColors = OutRawMesh.VertexInstanceAttributes().GetAttributesRef
		<FVector4>(MeshAttribute::VertexInstance::Color);
	TVertexInstanceAttributesRef<FVector2D> VertexInstanceUVs = OutRawMesh.VertexInstanceAttributes().GetAttributesRef<
		FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);

	OutRawMesh.ReserveNewVertices(NumVertexPositions);
	OutRawMesh.ReserveNewVertexInstances(NumWedges);
	OutRawMesh.ReserveNewPolygons(NumFaces);
	OutRawMesh.ReserveNewEdges(NumWedges);

	const int32 NumTexCoords = StaticMeshLOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
	VertexInstanceUVs.SetNumIndices(NumTexCoords);

	for (int32 SectionIndex = 0; SectionIndex < StaticMeshLOD.Sections.Num(); ++SectionIndex)
	{
		const FStaticMeshSection& Section = StaticMeshLOD.Sections[SectionIndex];
		FPolygonGroupID CurrentPolygonGroupID = OutRawMesh.CreatePolygonGroup();
		check(CurrentPolygonGroupID.GetValue() == SectionIndex);
		if (Materials.IsValidIndex(Section.MaterialIndex))
		{
			PolygonGroupImportedMaterialSlotNames[CurrentPolygonGroupID] = Materials[Section.MaterialIndex].
				ImportedMaterialSlotName;
		}
		else
		{
			PolygonGroupImportedMaterialSlotNames[CurrentPolygonGroupID] = FName(
				*(TEXT("MeshMergeMaterial_") + FString::FromInt(SectionIndex)));
		}
	}

	//Create the vertex
	for (int32 VertexIndex = 0; VertexIndex < NumVertexPositions; ++VertexIndex)
	{
		FVertexID VertexID = OutRawMesh.CreateVertex();
		VertexPositions[VertexID] = StaticMeshLOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
	}

	//Create the vertex instances
	for (int32 TriangleIndex = 0; TriangleIndex < NumFaces; ++TriangleIndex)
	{
		FPolygonGroupID CurrentPolygonGroupID = FPolygonGroupID::Invalid;
		for (int32 SectionIndex = 0; SectionIndex < StaticMeshLOD.Sections.Num(); ++SectionIndex)
		{
			const FStaticMeshSection& Section = StaticMeshLOD.Sections[SectionIndex];
			uint32 BeginTriangle = Section.FirstIndex / 3;
			uint32 EndTriangle = BeginTriangle + Section.NumTriangles;
			if ((uint32)TriangleIndex >= BeginTriangle && (uint32)TriangleIndex < EndTriangle)
			{
				CurrentPolygonGroupID = FPolygonGroupID(SectionIndex);
				break;
			}
		}
		check(CurrentPolygonGroupID != FPolygonGroupID::Invalid);

		FVertexID VertexIDs[3];
		TArray<FVertexInstanceID> VertexInstanceIDs;
		VertexInstanceIDs.SetNum(3);

		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			int32 WedgeIndex = StaticMeshLOD.IndexBuffer.GetIndex(TriangleIndex * 3 + Corner);
			FVertexID VertexID(WedgeIndex);
			FVertexInstanceID VertexInstanceID = OutRawMesh.CreateVertexInstance(VertexID);
			VertexIDs[Corner] = VertexID;
			VertexInstanceIDs[Corner] = VertexInstanceID;

			//NTBs
			FVector TangentX = StaticMeshLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(WedgeIndex);
			FVector TangentY = StaticMeshLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentY(WedgeIndex);
			FVector TangentZ = StaticMeshLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(WedgeIndex);
			VertexInstanceTangents[VertexInstanceID] = TangentX;
			VertexInstanceBinormalSigns[VertexInstanceID] = GetBasisDeterminantSign(TangentX, TangentY, TangentZ);
			VertexInstanceNormals[VertexInstanceID] = TangentZ;

			// Vertex colors
			if (StaticMeshLOD.VertexBuffers.ColorVertexBuffer.GetNumVertices() > 0)
			{
				VertexInstanceColors[VertexInstanceID] = FLinearColor(
					StaticMeshLOD.VertexBuffers.ColorVertexBuffer.VertexColor(WedgeIndex));
			}
			else
			{
				VertexInstanceColors[VertexInstanceID] = FLinearColor::White;
			}

			//Tex coord
			for (int32 TexCoodIdx = 0; TexCoodIdx < NumTexCoords; ++TexCoodIdx)
			{
				VertexInstanceUVs.Set(VertexInstanceID, TexCoodIdx,
				                      StaticMeshLOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(
					                      WedgeIndex, TexCoodIdx));
			}
		}
		//Create a polygon from this triangle
		const FPolygonID NewPolygonID = OutRawMesh.CreatePolygon(CurrentPolygonGroupID, VertexInstanceIDs);
	}
}

void AutoMergeHelper::ExtractSections(const UStaticMeshComponent* Component, int32 LODIndex,
                                      TArray<FSectionInfo>& OutSectionInfos)
{
	const UStaticMesh* StaticMesh = Component->GetStaticMesh();

	TArray<FName> MaterialSlotNames;
	for (const FStaticMaterial& StaticMaterial : StaticMesh->GetStaticMaterials())
	{
#if WITH_EDITOR
		MaterialSlotNames.Add(StaticMaterial.ImportedMaterialSlotName);
#else
		MaterialSlotNames.Add(StaticMaterial.MaterialSlotName);
#endif
	}

	for (const FStaticMeshSection& MeshSection : StaticMesh->GetRenderData()->LODResources[LODIndex].Sections)
	{
		if (MeshSection.NumTriangles == 0) continue;

		UMaterialInterface* Material = Component->GetMaterial(MeshSection.MaterialIndex);
		if (!Material || Material->GetMaterialResource(GMaxRHIFeatureLevel))
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		FSectionInfo SectionInfo;
		SectionInfo.Material = Material;
		SectionInfo.MaterialIndex = MeshSection.MaterialIndex;
		SectionInfo.MaterialSlotName = MaterialSlotNames.IsValidIndex(MeshSection.MaterialIndex)
			                               ? MaterialSlotNames[MeshSection.MaterialIndex]
			                               : NAME_None;
		SectionInfo.StartIndex = MeshSection.FirstIndex / 3;
		SectionInfo.EndIndex = SectionInfo.StartIndex + MeshSection.NumTriangles;

		if (MeshSection.bEnableCollision)
		{
			SectionInfo.EnabledProperties.Add(GET_MEMBER_NAME_CHECKED(FStaticMeshSection, bEnableCollision));
		}

		if (MeshSection.bCastShadow && Component->CastShadow)
		{
			SectionInfo.EnabledProperties.Add(GET_MEMBER_NAME_CHECKED(FStaticMeshSection, bCastShadow));
		}

		OutSectionInfos.Add(MoveTemp(SectionInfo));
	}
}
