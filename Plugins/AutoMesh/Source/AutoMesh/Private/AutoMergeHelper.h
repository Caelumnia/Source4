#pragma once

class AUTOMESH_API AutoMergeHelper
{
	using FMeshComponentArray = TArray<UStaticMeshComponent*>;
public:
	static UStaticMesh* MergeComponents(UObject* Outer, const FMeshComponentArray& MeshComponents, FVector Pivot);
	

private:
	static void ExportStaticMeshLOD(const FStaticMeshLODResources& StaticMeshLOD, const TArray<FStaticMaterial>& Materials, FMeshDescription& OutRawMesh);
	static void ExtractSections(const UStaticMeshComponent* Component, int32 LODIndex, TArray<FSectionInfo>& OutSectionInfos);
};
