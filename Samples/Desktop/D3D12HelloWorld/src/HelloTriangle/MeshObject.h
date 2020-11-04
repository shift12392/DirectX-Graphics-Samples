#pragma once


#include <vector>
#include "Common/MathHelper.h"
#include "DXSampleHelper.h"

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4 uv ;

	Vertex()
		:position(0,0,0)
		,normal(0, 0, 0)
		,tangent(0, 0, 0)
		,color(0, 0, 0, 0)
		,uv(0, 0, 0, 0)
	{

	}
	Vertex(DirectX::XMFLOAT3 InPosition, DirectX::XMFLOAT3 InNormal, DirectX::XMFLOAT3 InTangent, DirectX::XMFLOAT4 InColor,DirectX::XMFLOAT4 InUV)
		:position(InPosition), normal(InNormal),tangent(InTangent), color(InColor),uv(InUV)
	{

	}

	Vertex(float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v)
		:position(px,py,pz),normal(nx,ny,nz),tangent(tx,ty,tz),color(1,1,1,1), uv(u,v, u,v)
	{

	}

	static D3D12_INPUT_ELEMENT_DESC InputElementDesc[6];

};


struct ObjectData
{
	DirectX::XMFLOAT4X4 m_localToWorld = MathHelper::Identity4x4();
};

class StaticMesh
{
private:
	std::vector<Vertex>       m_vertexs;
	std::vector<UINT>         m_indices;
	std::wstring              m_name;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScratchAccelerationStructureBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ResultAccelerationStructureBuffer;
	bool                      m_isOpaque = true;

public:
	UINT StartInVertexBufferInBytes = 0;
	UINT StartInIndexBufferInBytes = 0;
	UINT StartInVertexBuffer = 0;
	UINT StartInIndexBuffer = 0;
public:
	UINT VertexBufferSize()
	{
		return m_vertexs.size() * sizeof(Vertex);
	}

	UINT IndexBufferSize()
	{
		return m_indices.size() * sizeof(UINT);
	}

	const std::vector<Vertex>&   GetVertexs() const { return m_vertexs; }
	const std::vector<uint32_t>& GetIndices() const { return m_indices; }
	const std::wstring& GetName() { return m_name; }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResultAccelerationStructureBuffer() { return m_ResultAccelerationStructureBuffer; }


	void BuildAccelerationStructure(ID3D12Device5* InDevice, ID3D12GraphicsCommandList4* InCommandList, ID3D12Resource* InVertexBuffer,ID3D12Resource* InIndexBuffer);
	void ReleaseTempResource()
	{
		m_ScratchAccelerationStructureBuffer.Reset();
	}
public:
	static StaticMesh* MakeCube(const std::wstring &InName, float x, float y, float z);
	static StaticMesh* MakePlane(const std::wstring &InName, float x,float y);



};

class StaticMeshObject
{
public:
	std::shared_ptr<StaticMesh> m_mesh;
	std::wstring m_name;

	D3D12_VERTEX_BUFFER_VIEW  m_vertexView = {};
	D3D12_INDEX_BUFFER_VIEW   m_indexView = {};

	DirectX::XMFLOAT4X4 m_localToWorld = MathHelper::Identity4x4();
	UINT m_indexInObjectBuffer = 0;
public:
	StaticMeshObject()
	{

	}
	StaticMeshObject(const std::wstring &inName)
		:m_name(inName)
	{
	}
	void SetLocalToWorld(const DirectX::XMFLOAT4X4& LocalToWorld)
	{
		m_localToWorld = LocalToWorld;
	}

	void SetStaticMesh(std::shared_ptr<StaticMesh> inStaticMesh,class Scene *InScene);
	std::shared_ptr<StaticMesh> GetStaticMesh() { return m_mesh; }
	void SetName(const std::wstring& inName) { m_name = inName; }
	std::wstring GetName() const {	return m_name; }
	uint32_t GetVertexNum() const
	{
		if (m_mesh)
		{
			return m_mesh->GetVertexs().size();
		}
		return 0;
	}
	uint32_t GetIndexNum() const
	{
		if (m_mesh)
		{
			return m_mesh->GetIndices().size();
		}
		return 0;
	}

	DirectX::XMFLOAT4X4 GetLocalToWorldTransform() const
	{
		return m_localToWorld;
	}

	void Draw(ID3D12GraphicsCommandList4* CommandList)
	{
		if (m_mesh)
		{
			CommandList->IASetVertexBuffers(0, 1, &m_vertexView);
			CommandList->IASetIndexBuffer(&m_indexView);
			CommandList->DrawIndexedInstanced(m_mesh->GetIndices().size(), 1, 0, 0, 0);
		}
		
	}
};

