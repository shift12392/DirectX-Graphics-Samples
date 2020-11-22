#pragma once

#include <unordered_map>
#include "MeshObject.h"
#include "ProceduralMesh.h"
#include "Camera.h"
#include "Common/MathHelper.h"



	class Scene : public NonCopyable
	{
	public:
		~Scene()
		{
			ReleaseTempResource();

			m_pInstanceDesc.Reset();
			m_topDestAccelerationStructureData.Reset();
			m_indexBuffer.Reset();
			m_vertexBuffer.Reset();
			m_cbvHeap.Reset();
		}
		struct ScenetData
		{
			DirectX::XMFLOAT4X4 m_ViewProj = MathHelper::Identity4x4();
			DirectX::XMFLOAT4X4  m_ProjectionToWorld = MathHelper::Identity4x4();
			DirectX::XMFLOAT4   m_CameraPos = DirectX::XMFLOAT4(0, 0, 0, 1);
			DirectX::XMFLOAT4   m_CameraInfo = DirectX::XMFLOAT4(0, 0, 0, 0);
			DirectX::XMFLOAT4   m_AmbientColor = DirectX::XMFLOAT4(1, 1, 1, 1);
			DirectX::XMFLOAT4   m_DirectionalLightDirection = DirectX::XMFLOAT4(1, 0, 0, 0);
			DirectX::XMFLOAT4   m_DirectionalLightColor = DirectX::XMFLOAT4(1, 1, 1, 1);
		};

		//场景用常量数据
		ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
		StructUploadBuffer<ScenetData> m_scenetBuffer;
		ScenetData m_sceneData;

		//每个StaticMeshObject用常量数据
		StructArrayUploadBuffer<ObjectData> m_allObjectBuffer;
		StructArrayUploadBuffer<SphereLightObject::SphereLightData>  m_allSphereLightBuffer;   //所有的灯光对象常量数据


		std::unordered_map<std::wstring, std::shared_ptr< Camera> >           m_cameras;

		std::unordered_map<std::wstring, std::shared_ptr<StaticMesh> >        m_staticMeshes;
		std::unordered_map<std::wstring, std::shared_ptr< StaticMeshObject> >  m_staticMeshObjects;
		//std::unordered_map<std::wstring, std::shared_ptr<ProceduralMeshObject>>      m_proceduralMeshObject;
		std::unordered_map<std::wstring, std::shared_ptr<SphereLightObject>>      m_sphereLightObjects;

		UINT64 m_allVertexNum    = 0;
		UINT64 m_allIndexNum     = 0;
		UINT64 m_allVertexSizeInBytes = 0;
		UINT64 m_allIndexSizeInBytes = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

		//加速结构
		Microsoft::WRL::ComPtr<ID3D12Resource> m_topScratchAccelerationStructureData;    // Scratch memory for AS builder
		Microsoft::WRL::ComPtr<ID3D12Resource> m_topDestAccelerationStructureData;       // Where the AS is
		Microsoft::WRL::ComPtr<ID3D12Resource> m_pInstanceDesc;                          // Hold the matrices of the instances

		Microsoft::WRL::ComPtr<ID3D12Resource> m_aabbBuffer;

		void Init(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList);
		void UpdateSceneData(const ScenetData& NewSceneData);
		void CollectStaticMesh(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList);
		void CreateStaticMeshObject(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList);
		void CollectLightObject(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList);

		void BuildTopAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* CommandList);
		void Update();
		void Draw(ID3D12GraphicsCommandList4* commandList);
		// 是否临时使用的资源。
		void ReleaseTempResource(); 
	public:
		static std::shared_ptr<Scene> CreateTestScene(ID3D12Device5* device, ID3D12GraphicsCommandList4* CommandList, float WindowWidth, float WindowHeight);
	private:
		void CalcAllMeshSize()
		{
			m_allVertexNum = 0;
		    m_allIndexNum = 0;
			m_allVertexSizeInBytes = 0;
			m_allIndexSizeInBytes = 0;

			auto Ite = m_staticMeshes.begin();
			for (; Ite != m_staticMeshes.end(); ++Ite)
			{
				m_allVertexNum += Ite->second->GetVertexs().size();
				m_allVertexSizeInBytes += Ite->second->VertexBufferSize();

				m_allIndexNum += Ite->second->GetIndices().size();
				m_allIndexSizeInBytes += Ite->second->IndexBufferSize();
			}
		}

		void CreateStaticMeshBuffer(ID3D12Device5* InDevice);
		void CreateProceduralMeshBuffer(ID3D12Device5* InDevice);
		void BuildStaticMeshAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* CommandList);
		void BuildProceduralMeshAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* CommandList);
	};




