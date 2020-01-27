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

#include "Structures.h"

static const D3D12_HEAP_PROPERTIES UploadHeapProperties =
{
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0, 0
};

static const D3D12_HEAP_PROPERTIES DefaultHeapProperties =
{
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0, 0
};

namespace D3DResources
{
	void Create_Buffer(D3D12Global &d3d, D3D12BufferCreateInfo& info, ID3D12Resource** ppResource);
	void Create_BackBuffer_RTV(D3D12Global &d3d, D3D12Resources &resources);
	void Create_BackBuffer_UAV(D3D12Global &d3d, D3D12Resources &resources);
	void Create_Descriptor_Heaps(D3D12Global &d3d, D3D12Resources &resources);
	void Create_PSO(D3D12Global &d3d, D3D12Resources &resources);
	void Create_ConstantBuffer(D3D12Global &d3d, D3D12Resources &resources, BandingConstants &constants);
	
	void Load_Shaders(D3D12Resources &resources, D3D12ShaderCompilerInfo &shaderCompiler);
	void Load_Blue_Noise_Textures(D3D12Global &d3d, D3D12Resources &resources, UINT num);

	void Upload_Texture(D3D12Global &d3d, ID3D12Resource* destResource, ID3D12Resource* srcResource, const TextureInfo &texture, UINT subresourceIndex);

	void Destroy(D3D12Resources &resources);
}

namespace D3DShaders
{
	void Init_Shader_Compiler(D3D12ShaderCompilerInfo &shaderCompiler);
	void Compile_Shader(D3D12ShaderCompilerInfo &compilerInfo, D3D12ShaderInfo &info, IDxcBlob** blob);
	void Destroy(D3D12ShaderCompilerInfo &shaderCompiler);
}

namespace D3D12
{
	void Create_Device(D3D12Global &d3d);
	void Create_CommandList(D3D12Global &d3d);
	void Create_Command_Queue(D3D12Global &d3d);
	void Create_Command_Allocator(D3D12Global &d3d);
	void Create_CommandList(D3D12Global &d3d);
	void Create_Fence(D3D12Global &d3d);
	void Create_Viewport(D3D12Global &d3d);
	void Create_Scissor(D3D12Global &d3d);
	void Create_SwapChain(D3D12Global &d3d, HWND &window);

	ID3D12RootSignature* Create_Root_Signature(D3D12Global &d3d, const D3D12_ROOT_SIGNATURE_DESC &desc);
	void Build_CmdList(D3D12Global &d3d, D3D12Resources &resources);

	void Reset_CommandList(D3D12Global &d3d);
	void Submit_CmdList(D3D12Global &d3d);
	void Present(D3D12Global &d3d);
	void WaitForGPU(D3D12Global &d3d);
	void MoveToNextFrame(D3D12Global &d3d);

	void Destroy(D3D12Global &d3d);
}
