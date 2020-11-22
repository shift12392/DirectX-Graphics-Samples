
#include "stdafx.h"
#include "ProceduralMesh.h"

UINT ProceduralMesh::AABBSize = ROUND_UP(sizeof(D3D12_RAYTRACING_AABB), D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT);

std::wstring SphereLightObject::s_HitGroupName = L"HitGroup_SphereLight";
std::wstring SphereLightObject::s_IntersectionName = L"Intersection_SphereLight";
std::wstring SphereLightObject::s_ClosestName = L"ClosestHit_SphereLight";

void ProceduralMesh::BuildAccelerationStructure(ID3D12Device5 * InDevice, ID3D12GraphicsCommandList4 * InCommandList, Microsoft::WRL::ComPtr<ID3D12Resource> AABBBuffer)
{

	// 首先构建底层加速结构

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
	geometryDesc.Flags = m_isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	geometryDesc.AABBs.AABBCount = 1;
	geometryDesc.AABBs.AABBs.StartAddress = AABBBuffer->GetGPUVirtualAddress() + OffsetInAABBBuffer;
	geometryDesc.AABBs.AABBs.StrideInBytes = AABBSize;


	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geometryDesc;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
	InDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &Info);

	//必须D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT字节对齐
	UINT64 resultSizeInBytes = ROUND_UP(Info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	UINT64 scratchDataSizeInBytes = ROUND_UP(Info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	//资源状态必须是D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	m_ResultAccelerationStructureBuffer = CreateDefaultBuffer(InDevice, resultSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	m_ScratchAccelerationStructureBuffer = CreateDefaultBuffer(InDevice, scratchDataSizeInBytes, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomDesc = {};
	bottomDesc.Inputs = inputs;
	bottomDesc.DestAccelerationStructureData = m_ResultAccelerationStructureBuffer->GetGPUVirtualAddress();
	bottomDesc.ScratchAccelerationStructureData = m_ScratchAccelerationStructureBuffer->GetGPUVirtualAddress();
	InCommandList->BuildRaytracingAccelerationStructure(&bottomDesc, 0, nullptr);

	// Wait for the builder to complete by setting a barrier on the resulting
	// buffer. This is particularly important as the construction of the top-level
	// hierarchy may be called right afterwards, before executing the command
	// list.
	// 通过在结果缓冲区上设置屏障来等待构建器完成。 这一点尤其重要，因为可以在执行命令列表之前立即调用顶层层次结构。
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_ResultAccelerationStructureBuffer.Get();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	InCommandList->ResourceBarrier(1, &uavBarrier);

}
