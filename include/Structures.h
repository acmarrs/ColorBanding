/* Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Common.h"

//--------------------------------------------------------------------------------------
// Global
//--------------------------------------------------------------------------------------

struct ConfigInfo 
{
	int			width = 640;
	int			height = 360;
	bool		vsync = false;
	HINSTANCE	instance = NULL;
};

struct BandingConstants
{
	DirectX::XMFLOAT3	lightPosition;
	float				noiseScale;
	DirectX::XMFLOAT3	color;
	UINT32				resolutionX;
	UINT32				frameNumber;
	int					useNoise;
	int					showNoise;
	int					noiseType;		// 0: white noise, 1: blue noise
};

struct TextureInfo
{
	std::vector<UINT8> pixels;
	int width = 0;
	int height = 0;
	int stride = 0;
	int offset = 0;
};

//--------------------------------------------------------------------------------------
// D3D12
//--------------------------------------------------------------------------------------

struct D3D12BufferCreateInfo
{
	UINT64 size = 0;
	UINT64 alignment = 0;
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

	D3D12BufferCreateInfo() {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags) : size(InSize), flags(InFlags) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		heapType(InHeapType),
		state(InState) {}
	
	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		flags(InFlags),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, UINT64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		alignment(InAlignment),
		heapType(InHeapType),
		flags(InFlags),
		state(InState) {}
};

struct D3D12ShaderCompilerInfo 
{
	dxc::DxcDllSupport		DxcDllHelper;
	IDxcCompiler*			compiler = nullptr;
	IDxcLibrary*			library = nullptr;
};

struct D3D12ShaderInfo 
{
	LPCWSTR		filename = nullptr;
	LPCWSTR		entryPoint = nullptr;
	LPCWSTR		targetProfile = nullptr;
	LPCWSTR*	arguments = nullptr;
	DxcDefine*	defines = nullptr;
	UINT32		argCount = 0;
	UINT32		defineCount = 0;

	D3D12ShaderInfo() {}
	D3D12ShaderInfo(LPCWSTR inFilename, LPCWSTR inEntryPoint, LPCWSTR inProfile) 
	{
		filename = inFilename;
		entryPoint = inEntryPoint;
		targetProfile = inProfile;
	}
};

struct D3D12Resources 
{
	ID3D12DescriptorHeap*						rtvHeap = nullptr;
	ID3D12DescriptorHeap*						descriptorHeap = nullptr;
	ID3D12DescriptorHeap*						uiDescriptorHeap = nullptr;

	ID3D12RootSignature*						rs = nullptr;
	ID3D12PipelineState*						pso = nullptr;
	
	ID3D12Resource*								bandingCB = nullptr;
	UINT8*										bandingCBStart = 0;

	IDxcBlob*									vsBytecode = nullptr;
	IDxcBlob*									psBytecode = nullptr;

	ID3D12Resource*								blueNoise = nullptr;
	ID3D12Resource*								blueNoiseUploadResource = nullptr;

	UINT										rtvDescSize = 0;
	UINT										cbvSrvUavDescSize = 0;
};

struct D3D12Global
{
	IDXGIFactory4*								factory = nullptr;
	IDXGIAdapter1*								adapter = nullptr;
	ID3D12Device5*								device = nullptr;
	ID3D12GraphicsCommandList4*					cmdList = nullptr;
	ID3D12CommandQueue*							cmdQueue = nullptr;
	ID3D12CommandAllocator*						cmdAlloc[2] = { nullptr, nullptr };

	IDXGISwapChain3*							swapChain = nullptr;
	ID3D12Resource*								backBuffer[2] = { nullptr, nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE					backBufferRTV[2] = { 0, 0 };

	ID3D12Fence*								fence = nullptr;
	UINT64										fenceValues[2] = { 0, 0 };
	HANDLE										fenceEvent = NULL;
	UINT										frameIndex = 0;

	D3D12_VIEWPORT								viewport;
	D3D12_RECT									scissor;

	int											width = 640;
	int											height = 360;
	bool										vsync = false;
};
