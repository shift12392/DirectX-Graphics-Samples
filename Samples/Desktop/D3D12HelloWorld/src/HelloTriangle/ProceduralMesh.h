#pragma once

#include <DirectXCollision.h>
#include "DXSampleHelper.h"
#include "Common/MathHelper.h"

class ProceduralMesh : public NonCopyable
{
public:
	static UINT AABBSize;

	//Axis-aligned bounding box
	DirectX::BoundingBox  m_box;
	D3D12_RAYTRACING_AABB m_aabb;

	std::wstring m_name;
	bool m_isOpaque = true;
	UINT OffsetInAABBBuffer = -1;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_ResultAccelerationStructureBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScratchAccelerationStructureBuffer;

	ProceduralMesh(const std::wstring& InName, DirectX::XMFLOAT3 InExtents)
		:m_name(InName), m_box(DirectX::XMFLOAT3(0,0,0),InExtents)
	{
		m_aabb.MinX =  - InExtents.x;
		m_aabb.MinY =  - InExtents.y;
		m_aabb.MinZ =  - InExtents.z;

		m_aabb.MaxX =  + InExtents.x;
		m_aabb.MaxY =  + InExtents.y;
		m_aabb.MaxZ =  + InExtents.z;
	}

	inline const std::wstring& GetName() const { return m_name; }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResultAccelerationStructureBuffer() { return m_ResultAccelerationStructureBuffer; }

	void ReleaseTempResource() { m_ScratchAccelerationStructureBuffer.Reset(); }

	void BuildAccelerationStructure(ID3D12Device5* InDevice, ID3D12GraphicsCommandList4* InCommandList, Microsoft::WRL::ComPtr<ID3D12Resource> AABBBuffer);
};

class ProceduralMeshObject : public NonCopyable
{
public:
	std::shared_ptr<ProceduralMesh> m_mesh;
	std::wstring m_name;
};


class SphereLightObject : public NonCopyable
{
public:
	struct SphereLightData
	{
		DirectX::XMFLOAT3 m_worldPos = DirectX::XMFLOAT3(0, 0, 0);
		float             m_radius = 1.0f;
		DirectX::XMFLOAT4 m_color = DirectX::XMFLOAT4(1, 1, 1, 1);
	}; 

public:
	static std::wstring s_HitGroupName;
	static std::wstring s_IntersectionName;
	static std::wstring s_ClosestName;
public:
	std::shared_ptr<ProceduralMesh> m_mesh;
	std::wstring m_name;
	SphereLightData m_data;
	DirectX::XMFLOAT4X4 m_localToWorld = MathHelper::Identity4x4();
	//DirectX::XMFLOAT4X4 m_worldToLocal = MathHelper::Identity4x4();
	UINT StartInAllSphereLightBuffer = -1;
public:
	SphereLightObject(const std::wstring& InName, DirectX::XMFLOAT3 InPosition,float InSphereRadius)
		:m_name(InName)
	{
		m_data.m_worldPos = InPosition;
		m_data.m_radius = InSphereRadius;

		std::wstring MeshName = L"ProceduralMesh_";
		MeshName += InName;
		DirectX::XMFLOAT3 Extents = DirectX::XMFLOAT3(InSphereRadius, InSphereRadius, InSphereRadius);
		m_mesh = std::make_shared<ProceduralMesh>(MeshName, Extents);

		//世界空间位置
		DirectX::XMMATRIX LocalToWorld = DirectX::XMMatrixTranslation(InPosition.x, InPosition.y, InPosition.z);
		DirectX::XMStoreFloat4x4(&m_localToWorld, DirectX::XMMatrixTranspose(LocalToWorld));

		//DirectX::XMVECTOR DetLocalToWorld = DirectX::XMMatrixDeterminant(LocalToWorld);
		//DirectX::XMMATRIX WorldToLocal = DirectX::XMMatrixInverse(&DetLocalToWorld, LocalToWorld);
		//DirectX::XMStoreFloat4x4(&m_worldToLocal, DirectX::XMMatrixTranspose(WorldToLocal));
	}

	const std::wstring& GetName() { return m_name; }
	const DirectX::XMFLOAT4X4& GetLocalToWorld() const { return m_localToWorld; }
	void SetColor(DirectX::XMFLOAT4 InColor) { m_data.m_color = InColor; }
};

