#include "stdafx.h"
#include "Scene.h"

#include <vector>


	void Scene::Init(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList)
	{
		m_scenetBuffer.CreateBuffer(device);
		m_scenetBuffer.CopyDataToGPU(m_sceneData);
	}

	void Scene::UpdateSceneData(const ScenetData & NewSceneData)
	{
		m_sceneData = NewSceneData;
		m_scenetBuffer.CopyDataToGPU(m_sceneData);
	}

	void Scene::Update()
	{
		//更新相机

		m_scenetBuffer.CopyDataToGPU(m_sceneData);
	}

	void Scene::Draw(ID3D12GraphicsCommandList4* commandList)
	{
		for (auto Ite = m_staticMeshObjects.begin(); Ite != m_staticMeshObjects.end(); ++Ite)
		{
			commandList->SetGraphicsRootConstantBufferView(1, m_allObjectBuffer.GetElementGpuVirtualAddress(Ite->second->m_indexInObjectBuffer));
			Ite->second->Draw(commandList);
		}
	}

	void Scene::ReleaseTempResource()
	{
		m_topScratchAccelerationStructureData.Reset();
		for (auto Ite = m_staticMeshes.begin(); Ite != m_staticMeshes.end(); ++Ite)
		{
			Ite->second->ReleaseTempResource();
		}
	}

	void Scene::BuildBottomAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* CommandList)
	{
		for (auto Ite = m_staticMeshes.begin(); Ite != m_staticMeshes.end(); ++Ite)
		{
			Ite->second->BuildAccelerationStructure(device, CommandList, m_vertexBuffer.Get(), m_indexBuffer.Get());
		}
	}

	void Scene::CreateMeshBuffer(ID3D12Device5* InDevice)
	{
		m_vertexBuffer = CreateUploadBuffer(InDevice, m_allVertexSizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_indexBuffer = CreateUploadBuffer(InDevice, m_allIndexSizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ);

		UINT8* vertexBufferGPUAddress = nullptr;
		UINT8* indexBufferGPUAddress = nullptr;
		ThrowIfFailed(m_vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertexBufferGPUAddress)));
		ThrowIfFailed(m_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexBufferGPUAddress)));

		UINT CurrentInVertexBufferInBytes = 0;
		UINT CurrentInIndexBufferInBytes = 0;
		UINT CurrentInVertexBuffer = 0;
		UINT CurrentInIndexBuffer = 0;
		for (auto Ite = m_staticMeshes.begin(); Ite != m_staticMeshes.end(); ++Ite)
		{
			std::shared_ptr<StaticMesh> Mesh = Ite->second;
			UINT CurrentMeshVertexSize = Mesh->VertexBufferSize();
			UINT CurrentMeshIndexSize = Mesh->IndexBufferSize();

			Mesh->StartInVertexBuffer = CurrentInVertexBuffer;
			CurrentInVertexBuffer += Mesh->GetVertexs().size();

			Mesh->StartInIndexBuffer = CurrentInIndexBuffer;
			CurrentInIndexBuffer += Mesh->GetIndices().size();

			Mesh->StartInVertexBufferInBytes = CurrentInVertexBufferInBytes;
			memcpy(vertexBufferGPUAddress + Mesh->StartInVertexBufferInBytes, Mesh->GetVertexs().data(), CurrentMeshVertexSize);
			CurrentInVertexBufferInBytes += CurrentMeshVertexSize;

			Mesh->StartInIndexBufferInBytes = CurrentInIndexBufferInBytes;
			memcpy(indexBufferGPUAddress + Mesh->StartInIndexBufferInBytes, Mesh->GetIndices().data(), CurrentMeshIndexSize);
			CurrentInIndexBufferInBytes += CurrentMeshIndexSize;
		}

		m_vertexBuffer->Unmap(0, nullptr);
		m_indexBuffer->Unmap(0, nullptr);
	}

	void Scene::CollectStaticMesh(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList)
	{
		//创建网格体
		std::shared_ptr<StaticMesh> planeMesh(StaticMesh::MakePlane(L"StaticMesh_Plane", 100.0f, 100.0f));
		std::shared_ptr<StaticMesh> planeMesh1(StaticMesh::MakePlane(L"StaticMesh_Plane1", 100.0f, 100.0f));
		std::shared_ptr<StaticMesh> CubeMesh(StaticMesh::MakeCube(L"StaticMesh_Cube", 50.0f, 50.0f,50.0f));
		
		m_staticMeshes.insert({ planeMesh->GetName(), planeMesh });
		m_staticMeshes.insert({ CubeMesh->GetName(), CubeMesh });
		m_staticMeshes.insert({ planeMesh1->GetName(), planeMesh1 });

		CalcAllMeshSize();


		//创建顶点和索引BUFFER。
		CreateMeshBuffer(device);
		//给所有网格体构建底层加速结构
		BuildBottomAccelerationStructure(device, commandList);
	}

	void Scene::BuildTopAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* CommandList)
	{
		// 构建顶级加速结构

        //每个实例必须D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT对齐。
		UINT64 instanceSizeInBytes = ROUND_UP(sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);
		UINT64 allInstanceSizeInBytes = CalculateConstantBufferByteSize(instanceSizeInBytes * m_staticMeshObjects.size());
		m_pInstanceDesc = CreateUploadBuffer(device, allInstanceSizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ);

		D3D12_RAYTRACING_INSTANCE_DESC *pInstanceDesc = nullptr;
		ThrowIfFailed(m_pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&pInstanceDesc)));
		ZeroMemory(pInstanceDesc, allInstanceSizeInBytes);

		UINT i = 0;
		for (auto Ite = m_staticMeshObjects.begin(); Ite != m_staticMeshObjects.end(); ++Ite)
		{
			pInstanceDesc[i].InstanceID = i;
			pInstanceDesc[i].InstanceMask = 0xFF;
			pInstanceDesc[i].InstanceContributionToHitGroupIndex = i;
			pInstanceDesc[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			pInstanceDesc[i].AccelerationStructure = Ite->second->GetStaticMesh()->GetResultAccelerationStructureBuffer()->GetGPUVirtualAddress();

			memcpy(&(pInstanceDesc[i].Transform), &(Ite->second->GetLocalToWorldTransform()), sizeof(pInstanceDesc[i].Transform));

			++i;
		}
		m_pInstanceDesc->Unmap(0, nullptr);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS TopInputs = {};
		TopInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		TopInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		TopInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		TopInputs.NumDescs = m_staticMeshObjects.size();

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO TopInfo = {};
		device->GetRaytracingAccelerationStructurePrebuildInfo(&TopInputs, &TopInfo);

		UINT64 resultSizeInBytes = ROUND_UP(TopInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		UINT64 scratchDataSizeInBytes = ROUND_UP(TopInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		m_topDestAccelerationStructureData = CreateDefaultBuffer(device, resultSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		m_topScratchAccelerationStructureData = CreateDefaultBuffer(device, scratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		TopInputs.InstanceDescs = m_pInstanceDesc->GetGPUVirtualAddress();
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topDesc = {};
		topDesc.Inputs = TopInputs;
		topDesc.ScratchAccelerationStructureData = m_topScratchAccelerationStructureData->GetGPUVirtualAddress();
		topDesc.DestAccelerationStructureData = m_topDestAccelerationStructureData->GetGPUVirtualAddress();

		CommandList->BuildRaytracingAccelerationStructure(&topDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER uavBarrier2;
		uavBarrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier2.UAV.pResource = m_topDestAccelerationStructureData.Get();
		uavBarrier2.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		CommandList->ResourceBarrier(1, &uavBarrier2);
	}

	void Scene::CreateStaticMeshObject(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList)
	{
		//创建网格对象
		std::shared_ptr<StaticMeshObject> planeMeshObject(new StaticMeshObject(L"StaticMeshObject_Plane"));
		planeMeshObject->SetStaticMesh(m_staticMeshes[L"StaticMesh_Plane"], this);
		DirectX::XMMATRIX LocalToWorld = DirectX::XMMatrixTranslation(0.0f, -100.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&planeMeshObject->m_localToWorld, DirectX::XMMatrixTranspose(LocalToWorld));

		std::shared_ptr<StaticMeshObject> planeMeshObject1(new StaticMeshObject(L"StaticMeshObject_Plane1"));
		planeMeshObject1->SetStaticMesh(m_staticMeshes[L"StaticMesh_Plane1"], this);
		LocalToWorld = DirectX::XMMatrixTranslation(0.0f, 100.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&planeMeshObject1->m_localToWorld, DirectX::XMMatrixTranspose(LocalToWorld));

		std::shared_ptr<StaticMeshObject> CubeMeshObject(new StaticMeshObject(L"StaticMeshObject_Cube"));
		CubeMeshObject->SetStaticMesh(m_staticMeshes[L"StaticMesh_Cube"], this);
		LocalToWorld = DirectX::XMMatrixTranslation(200.0f, 0.0f, 0.0f);
		DirectX::XMStoreFloat4x4(&CubeMeshObject->m_localToWorld, DirectX::XMMatrixTranspose(LocalToWorld));

		m_staticMeshObjects.insert({ planeMeshObject->GetName(), planeMeshObject });
		m_staticMeshObjects.insert({ CubeMeshObject->GetName(),CubeMeshObject });
		m_staticMeshObjects.insert({ planeMeshObject1->GetName(), planeMeshObject1 });

		//
		UINT i = 0;
		m_allObjectBuffer.CreateBuffer(device, m_staticMeshObjects.size());
		for (auto Ite = m_staticMeshObjects.begin(); Ite != m_staticMeshObjects.end(); ++Ite)
		{
			Ite->second->m_indexInObjectBuffer = i;

			ObjectData NewData;
			NewData.m_localToWorld = Ite->second->m_localToWorld;
			m_allObjectBuffer.CopyDataToGPU(NewData, i);

			++i;
		}

		BuildTopAccelerationStructure(device, commandList);
	}

	std::shared_ptr<Scene> Scene::CreateTestScene(ID3D12Device5* device, ID3D12GraphicsCommandList4* CommandList,float WindowWidth,float WindowHeight)
	{
		//注意：不建议使用上载堆来传输静态数据，例如vert缓冲区。静态网格数据建议使用默认堆。

		std::shared_ptr<Scene> NewScene(new Scene());
		NewScene->Init(device, CommandList);

		//创建相机
		std::shared_ptr<Camera> NewCamera = Camera::CreateProjectionCamera(L"MainCamera", WindowWidth / WindowHeight);
		NewScene->m_cameras.insert({ NewCamera->m_name,NewCamera });

		ScenetData NewSceneData;
		NewSceneData.m_ViewProj = NewCamera->m_viewProj;
		NewSceneData.m_ProjectionToWorld = NewCamera->m_ProjToWorld;
		NewSceneData.m_CameraPos = NewCamera->m_pos;
		NewSceneData.m_CameraInfo = DirectX::XMFLOAT4(NewCamera->m_aspectRatio, NewCamera->m_near, NewCamera->m_far, 0.0f);
		//设置灯光
		NewSceneData.m_AmbientColor = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		NewSceneData.m_DirectionalLightColor = DirectX::XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);
		NewSceneData.m_DirectionalLightDirection = DirectX::XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
		
		NewScene->UpdateSceneData(NewSceneData);

		//收集网格体
		NewScene->CollectStaticMesh(device,CommandList);

		//创建对象
		NewScene->CreateStaticMeshObject(device, CommandList);

		return NewScene;
	}



