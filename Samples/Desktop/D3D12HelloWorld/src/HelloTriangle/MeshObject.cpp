#include "stdafx.h"
#include "MeshObject.h"
#include "Scene.h"

D3D12_INPUT_ELEMENT_DESC Vertex::InputElementDesc[6] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 52, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 60, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};


const wchar_t* StaticMeshObject::s_HitGroup_Triangle = L"HitGroup_Triangle";
const wchar_t* StaticMeshObject::s_ClosestHit_Triangle = L"ClosestHit_Triangle";
const wchar_t* StaticMeshObject::s_HitGroup_TriangleShadow = L"HitGroup_TriangleShadow";


void StaticMesh::BuildAccelerationStructure(ID3D12Device5 * InDevice, ID3D12GraphicsCommandList4 * InCommandList, ID3D12Resource* InVertexBuffer, ID3D12Resource* InIndexBuffer)
{
	// 首先构建底层加速结构

	//在加速结构构建期间，将以行为主的布局的3x4仿射变换矩阵的地址应用于VertexBuffer中的顶点。 不修改VertexBuffer的内容。 如果使用2D顶点格式，则在假定第三个顶点分量为零的情况下应用转换。
	//如果Transform3x4为NULL，则不会对顶点进行变换。 使用Transform3x4可能会导致加速结构构建的计算和/或内存需求增加。
	//指向的内存必须处于状态D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE。 该地址必须对齐为16个字节，定义为D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT。

	//IndexBuffer中索引的格式。 必须为以下之一：
	//    DXGI_FORMAT_UNKNOWN - 当IndexBuffer为NULL时
	//    DXGI_FORMAT_R32_UINT
	//    DXGI_FORMAT_R16_UINT
	// 顶点索引数组。 如果为NULL，则三角形不索引。 与graphics一样，地址必须与IndexFormat的大小对齐。
	// 指向的内存必须处于状态D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE。 请注意，如果应用程序希望在图形输入装配阶段和光线跟踪加速结构构建输入之间共享索引缓冲区输入，则它始终可以同时将资源置于读取状态的组合中，例如 D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE。
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Flags = m_isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	geometryDesc.Triangles.VertexBuffer.StartAddress = InVertexBuffer->GetGPUVirtualAddress() + StartInVertexBufferInBytes;
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geometryDesc.Triangles.VertexCount = m_vertexs.size();
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = InIndexBuffer->GetGPUVirtualAddress() + StartInIndexBufferInBytes;
	geometryDesc.Triangles.IndexCount = m_indices.size();
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

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

StaticMesh * StaticMesh::MakeCube(const std::wstring &InName, float x, float y, float z)
{
	assert(x > 0.0f);
	assert(y > 0.0f);
	assert(z > 0.0f);

	StaticMesh *Cube = new StaticMesh();
	Cube->m_name = InName;
	float y2 = 0.5f*y;
	float z2 = 0.5f*z;
	float x2 = 0.5f*x;

	Vertex v[24];
	
	//注：切线没有设置

	// Fill in the front face vertex data.
	v[0] = Vertex(-x2, -y2, +z2,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[1] = Vertex(-x2, +y2, +z2,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[2] = Vertex(-x2, +y2, -z2,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[3] = Vertex(-x2, -y2, -z2,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(+x2, -y2, +z2,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[5] = Vertex(+x2, +y2, +z2,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[6] = Vertex(+x2, +y2, -z2,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[7] = Vertex(+x2, -y2, -z2,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the top face vertex data.
	v[8]  = Vertex( +x2, -y2, +z2, 0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[9]  = Vertex( +x2, +y2, +z2, 0.0f,  0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[10] = Vertex( -x2, +y2, +z2, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[11] = Vertex( -x2, -y2, +z2, 0.0f,  0.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(+x2, -y2, -z2, 0.0f,  0.0f, -1.0f,   0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[13] = Vertex(+x2, +y2, -z2, 0.0f,  0.0f, -1.0f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[14] = Vertex(-x2, +y2, -z2, 0.0f,  0.0f, -1.0f,   0.0f, 0.0f, 0.0f,  0.0f, 1.0f);
	v[15] = Vertex(-x2, -y2, -z2, 0.0f,  0.0f, -1.0f,   0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-x2, -y2, +z2,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[17] = Vertex(+x2, -y2, +z2,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[18] = Vertex(+x2, -y2, -z2,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[19] = Vertex(-x2, -y2, -z2, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(-x2, +y2, +z2,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[21] = Vertex(+x2, +y2, +z2,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f);
	v[22] = Vertex(+x2, +y2, -z2,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[23] = Vertex(-x2, +y2, -z2,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	Cube->m_vertexs.assign(&v[0], &v[24]);

	for (auto Ite = Cube->m_vertexs.begin(); Ite != Cube->m_vertexs.end(); ++Ite)
	{
		Ite->color = DirectX::XMFLOAT4(1, 1, 1, 1);
	}

	//
	// Create the indices.
	//

	uint32_t i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 6; i[8] = 5;
	i[9] = 4; i[10] = 7; i[11] = 6;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 14; i[20] = 13;
	i[21] = 12; i[22] = 15; i[23] = 14;

	// Fill in the left face index data
	i[24] = 16; i[25] = 18; i[26] = 17;
	i[27] = 16; i[28] = 19; i[29] = 18;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	Cube->m_indices.assign(&i[0], &i[36]);

	return Cube;
}

StaticMesh* StaticMesh::MakePlane(const std::wstring &InName,float x, float y)
{
	assert(x > 0);
	assert(y > 0);

	float xHalf = x * 0.5f;
	float yHalf = y * 0.5f;
	

	StaticMesh *Plane = new StaticMesh();
	Plane->m_name = InName;

	//注：先不用切线方向
	Vertex v[4];
	v[0] = Vertex(DirectX::XMFLOAT3(xHalf, -yHalf, 0.0f),  DirectX::XMFLOAT3( 0, 0, 1), DirectX::XMFLOAT3( 0, 0, 0), DirectX::XMFLOAT4( 1,1,1,1), DirectX::XMFLOAT4(0, 0, 0, 0));
	v[1] = Vertex(DirectX::XMFLOAT3(xHalf, yHalf, 0.0f),   DirectX::XMFLOAT3(0, 0, 1), DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT4(1, 1, 1, 1), DirectX::XMFLOAT4(1, 0, 0, 0));
	v[2] = Vertex(DirectX::XMFLOAT3(-xHalf, yHalf, 0.0f),  DirectX::XMFLOAT3(0, 0, 1), DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT4(1, 1, 1, 1), DirectX::XMFLOAT4(0, 1, 0, 0));
	v[3] = Vertex(DirectX::XMFLOAT3(-xHalf, -yHalf, 0.0f), DirectX::XMFLOAT3(0, 0, 1), DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT4(1, 1, 1, 1), DirectX::XMFLOAT4(1, 1, 0, 0));

	Plane->m_vertexs.assign(&v[0], &v[4]);


	uint32_t i[6];
	i[0] = 0;	i[1] = 1;	i[2] = 2;
	i[3] = 0;	i[4] = 2;	i[5] = 3;

	Plane->m_indices.assign(&i[0], &i[6]);

	return Plane;
}


void StaticMeshObject::SetStaticMesh(std::shared_ptr<StaticMesh> inStaticMesh, Scene *InScene)
{
	m_mesh = inStaticMesh;

	m_vertexView.BufferLocation = InScene->m_vertexBuffer->GetGPUVirtualAddress() + m_mesh->StartInVertexBufferInBytes;
	m_vertexView.SizeInBytes = m_mesh->VertexBufferSize();
	m_vertexView.StrideInBytes = sizeof(Vertex);

	m_indexView.BufferLocation = InScene->m_indexBuffer->GetGPUVirtualAddress() + m_mesh->StartInIndexBufferInBytes;
	m_indexView.Format = DXGI_FORMAT_R32_UINT;
	m_indexView.SizeInBytes = m_mesh->IndexBufferSize();

}