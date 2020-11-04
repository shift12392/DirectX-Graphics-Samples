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
#include <stdexcept>
#include <cassert>

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
using Microsoft::WRL::ComPtr;

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment)                                         \
  ( ((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    WCHAR* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}

inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size)
{
    using namespace Microsoft::WRL;

    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
    extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile = nullptr;

    Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
    if (file.Get() == INVALID_HANDLE_VALUE)
    {
        throw std::exception();
    }

    FILE_STANDARD_INFO fileInfo = {};
    if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        throw std::exception();
    }

    if (fileInfo.EndOfFile.HighPart != 0)
    {
        throw std::exception();
    }

    *data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
    *size = fileInfo.EndOfFile.LowPart;

    if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
    {
        throw std::exception();
    }

    return S_OK;
}

// Assign a name to the object to aid with debugging.
#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
    pObject->SetName(name);
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
    WCHAR fullName[50];
    if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
    {
        pObject->SetName(fullName);
    }
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
    // Constant buffer size is required to be aligned.
    return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

inline UINT64 CalculateConstantBufferByteSize(UINT64 byteSize)
{
	// Constant buffer size is required to be aligned.
	return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr;

    Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    return byteCode;
}
#endif

// Resets all elements in a ComPtr array.
template<class T>
void ResetComPtrArray(T* comPtrArray)
{
    for (auto &i : *comPtrArray)
    {
        i.Reset();
    }
}


// Resets all elements in a unique_ptr array.
template<class T>
void ResetUniquePtrArray(T* uniquePtrArray)
{
    for (auto &i : *uniquePtrArray)
    {
        i.reset();
    }
}

inline ID3D12Resource* CreateUploadBuffer(ID3D12Device5 *InDevice,
	UINT64 InBufferSize,
	D3D12_RESOURCE_STATES InResourceState = D3D12_RESOURCE_STATE_COMMON,
	D3D12_RESOURCE_FLAGS InResourceFlag = D3D12_RESOURCE_FLAG_NONE)
{
	ID3D12Resource *resource = nullptr;

	ThrowIfFailed(InDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(InBufferSize, InResourceFlag), InResourceState, nullptr, IID_PPV_ARGS(&resource)));

	return resource;
}

inline ID3D12Resource* CreateDefaultBuffer(ID3D12Device5 *InDevice,
	UINT64 InBufferSize,
	D3D12_RESOURCE_STATES InResourceState = D3D12_RESOURCE_STATE_COMMON,
	D3D12_RESOURCE_FLAGS InResourceFlag = D3D12_RESOURCE_FLAG_NONE)
{
	ID3D12Resource *resource = nullptr;

	ThrowIfFailed(InDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT,0,0), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(InBufferSize, InResourceFlag), InResourceState, nullptr, IID_PPV_ARGS(&resource)));

	return resource;
}

template<typename T>
class StructUploadBuffer
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	UINT64 m_bufferSize = 0;
	T *m_data = nullptr;

	~StructUploadBuffer()
	{
		if (m_data != nullptr)
		{
			m_resource->Unmap(0, nullptr);
			m_data = nullptr;
		}
	}

	void CreateBuffer(ID3D12Device5 *InDevice)
	{
		m_bufferSize = CalculateConstantBufferByteSize(sizeof(T));
		m_resource = CreateUploadBuffer(InDevice, m_bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
		ThrowIfFailed( m_resource->Map(0, nullptr, reinterpret_cast<void**>(&m_data)) );
	}

	void CopyDataToGPU(const T& InData)
	{
		memcpy(m_data, &InData, m_bufferSize);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() { return m_resource->GetGPUVirtualAddress(); }
	UINT64 GetBufferSize() { return m_bufferSize; }
};


template<typename T>
class StructArrayUploadBuffer
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	UINT64 m_bufferSize = 0;
	UINT   m_elementSize = 0;
	UINT   m_num = 0;
	UINT8 *m_data = nullptr;

	~StructArrayUploadBuffer()
	{
		if (m_data != nullptr)
		{
			m_resource->Unmap(0, nullptr);
			m_data = nullptr;
		}

		m_bufferSize = 0;
		m_num = 0;
	}

	void CreateBuffer(ID3D12Device5 *InDevice,UINT Num)
	{
		assert(Num > 0);
		m_num = Num;
		m_elementSize = CalculateConstantBufferByteSize(sizeof(T));
		m_bufferSize = Num * m_elementSize;
		m_resource = CreateUploadBuffer(InDevice, m_bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
		ThrowIfFailed(m_resource->Map(0, nullptr, reinterpret_cast<void**>(&m_data)));
	}

	void CopyDataToGPU(const T& InData,UINT InElement)
	{
		assert(InElement >= 0);
		assert(InElement < m_num);
		memcpy( m_data + InElement * m_elementSize, &InData, sizeof(T));
	}

	inline D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() { return m_resource->GetGPUVirtualAddress(); }
	inline UINT64 GetBufferSize() { return m_bufferSize; }
	inline UINT   GetElementSize() { return m_elementSize; }
	inline D3D12_GPU_VIRTUAL_ADDRESS GetElementGpuVirtualAddress(UINT InElement) { return GetGpuVirtualAddress() + InElement * GetElementSize(); }
};

