//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"

#include <dxcapi.h>

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12HelloTriangle : public DXSample
{
public:
    D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
	virtual void OnKeyUp(UINT8 key)
	{
		if (key == VK_SPACE)
		{
			m_raster = !m_raster;
		}
	}
	
private:
    static const UINT FrameCount = 2;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

	struct SceneData
	{
		DirectX::XMFLOAT4  m_ambientColor;
	};

	bool m_raster = true;
	DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device5> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rasterizationRootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList4> m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

	SceneData m_sceneData;
	StructUploadBuffer<SceneData> m_sceneDataGPUBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_outputResource;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtxHeap;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_bottomScratchAccelerationStructureData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_bottomDestAccelerationStructureData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_topDestAccelerationStructureData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_topScratchAccelerationStructureData;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instance;

	ComPtr<IDxcBlob> m_rayGenLibrary;
	ComPtr<IDxcBlob> m_hitLibrary;
	ComPtr<IDxcBlob> m_missLibrary;

	ComPtr<ID3D12RootSignature> m_rayGenSignature;
	ComPtr<ID3D12RootSignature> m_hitSignature;
	ComPtr<ID3D12RootSignature> m_missSignature;

	// Ray tracing pipeline state
	ComPtr<ID3D12StateObject> m_rtStateObject;
	// Ray tracing pipeline state properties, retaining the shader identifiers
	// to use in the Shader Binding Table
	ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

	nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
	ComPtr<ID3D12Resource> m_sbtStorage;

	void CreateShaderBindingTable();

    void LoadPipeline();
    void LoadAssets();
	void CheckRaytracingSupport();
	void CreateScene();
	void CreateMeshes();
	void CreateRootSignature();
	// #DXR
	ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	ComPtr<ID3D12RootSignature> CreateMissSignature();
	ComPtr<ID3D12RootSignature> CreateHitSignature();

	void CreateShaderResource();
	void CreateRaytracingPipeline();
	void CreateGraphicsPipelineState();

    void PopulateCommandList();
    void WaitForPreviousFrame();


	Microsoft::WRL::ComPtr<ID3D12RootSignature> SerializeRootSignature(CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc);
};
