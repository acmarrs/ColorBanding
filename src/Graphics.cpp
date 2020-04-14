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

#include <wrl.h>
#include <atlcomcli.h>

#include "Graphics.h"
#include "Utils.h"

using namespace std;
using Microsoft::WRL::ComPtr;

//--------------------------------------------------------------------------------------
// Resource Functions
//--------------------------------------------------------------------------------------

namespace D3DResources
{

/**
* Create a GPU buffer resource.
*/
void Create_Buffer(D3D12Global &d3d, D3D12BufferCreateInfo &info, ID3D12Resource** ppResource)
{
    D3D12_HEAP_PROPERTIES heapDesc = {};
    heapDesc.Type = info.heapType;
    heapDesc.CreationNodeMask = 1;
    heapDesc.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = info.alignment;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Width = info.size;
    resourceDesc.Flags = info.flags;

    // Create the GPU resource
    HRESULT hr = d3d.device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &resourceDesc, info.state, nullptr, IID_PPV_ARGS(ppResource));
    Utils::Validate(hr, L"Error: failed to create buffer resource!");
}

/**
* Create the back buffer RTVs.
*/
void Create_BackBuffer_RTV(D3D12Global &d3d, D3D12Resources &resources)
{
    HRESULT hr;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = resources.rtvHeap->GetCPUDescriptorHandleForHeapStart();

    // Create an RTV for each back buffer
    for (UINT n = 0; n < 2; n++)
    {
        hr = d3d.swapChain->GetBuffer(n, IID_PPV_ARGS(&d3d.backBuffer[n]));
        if (FAILED(hr))
        {
            throw runtime_error("Failed to get swap chain buffer!");
        }

        d3d.backBufferRTV[n] = rtvHandle;
        d3d.device->CreateRenderTargetView(d3d.backBuffer[n], nullptr, d3d.backBufferRTV[n]);
#if NAME_D3D_RESOURCES
        if (n == 0)
        {
            d3d.backBuffer[n]->SetName(L"Back Buffer 0 RTV");
        }
        else
        {
            d3d.backBuffer[n]->SetName(L"Back Buffer 1 RTV");
        }
#endif
        rtvHandle.ptr += resources.rtvDescSize;
    }
}

/**
* Create the RTV and UI descriptor heaps.
*/
void Create_Descriptor_Heaps(D3D12Global &d3d, D3D12Resources &resources)
{
    // Describe the RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.NumDescriptors = 2;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    // Create the RTV descriptor heap
    HRESULT hr = d3d.device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&resources.rtvHeap));
    Utils::Validate(hr, L"Error: failed to create RTV descriptor heap!");
#if NAME_D3D_RESOURCES
    resources.rtvHeap->SetName(L"RTV Descriptor Heap");
#endif

    resources.rtvDescSize = d3d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Describe the descriptor heap
    // 1 SRV for the blue noise texture
    // 1 SRV for the blue noise texture array
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 2;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
    // Create the descriptor heap
    hr = d3d.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&resources.descriptorHeap));
    Utils::Validate(hr, L"Error: failed to create the descriptor heap!");
#if NAME_D3D_RESOURCES
    resources.descriptorHeap->SetName(L"Descriptor Heap");
#endif

    resources.cbvSrvUavDescSize = d3d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Describe the UI descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC uiDesc = {};
    uiDesc.NumDescriptors = 1;
    uiDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uiDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    // Create the UI descriptor heap
    hr = d3d.device->CreateDescriptorHeap(&uiDesc, IID_PPV_ARGS(&resources.uiDescriptorHeap));
    Utils::Validate(hr, L"Error: failed to create UI descriptor heap!");
#if NAME_D3D_RESOURCES
    resources.uiDescriptorHeap->SetName(L"UI Descriptor Heap");
#endif
}

/**
 * Create the graphics PSO.
 */
void Create_PSO(D3D12Global &d3d, D3D12Resources &resources)
{
    D3D12_SHADER_BYTECODE vs;
    vs.BytecodeLength = resources.vsBytecode->GetBufferSize();
    vs.pShaderBytecode = resources.vsBytecode->GetBufferPointer();

    D3D12_SHADER_BYTECODE ps;
    ps.BytecodeLength = resources.psBytecode->GetBufferSize();
    ps.pShaderBytecode = resources.psBytecode->GetBufferPointer();

    // Describe the rasterizer states
    D3D12_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

    const D3D12_RENDER_TARGET_BLEND_DESC defaultBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    D3D12_INPUT_ELEMENT_DESC defaultInputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0] = defaultBlendDesc;

    // Constant Buffer Root Parameter
    D3D12_ROOT_PARAMETER param0 = {};
    param0.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param0.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    param0.Descriptor.RegisterSpace = 0;
    param0.Descriptor.ShaderRegister = 0;

    // Describe the descriptor table
    D3D12_DESCRIPTOR_RANGE range;
    range.BaseShaderRegister = 0;
    range.NumDescriptors = 2;
    range.RegisterSpace = 0;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER param1 = {};
    param1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param1.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    param1.DescriptorTable.NumDescriptorRanges = 1;
    param1.DescriptorTable.pDescriptorRanges = &range;

    D3D12_ROOT_PARAMETER rootParams[2] = { param0, param1 };

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(rootParams);
    rsDesc.pParameters = rootParams;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    resources.rs = D3D12::Create_Root_Signature(d3d, rsDesc);

    // Describe and create the PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.InputLayout = { defaultInputElementDescs, _countof(defaultInputElementDescs) };
    desc.pRootSignature = resources.rs;
    desc.VS = vs;
    desc.PS = ps;
    desc.RasterizerState = rasterDesc;
    desc.BlendState = blendDesc;
    desc.SampleMask = UINT_MAX;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;

    // Create the PSO
    HRESULT hr = d3d.device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&resources.pso));
    Utils::Validate(hr, L"Error: failed to create the graphics PSO!");
#if NAME_D3D_RESOURCES
    resources.pso->SetName(L"PSO");
#endif
}

/**
 * Create the constant buffer.
 */
void Create_ConstantBuffer(D3D12Global &d3d, D3D12Resources &resources, BandingConstants &constants)
{
    UINT size = ALIGN(256, sizeof(BandingConstants));

    // Create the banding constant buffer
    D3D12BufferCreateInfo desc = D3D12BufferCreateInfo(size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    Create_Buffer(d3d, desc, &resources.bandingCB);
#if NAME_D3D_RESOURCES
    resources.bandingCB->SetName(L"Banding Constant Buffer");
#endif

    // Map the banding constant buffer and copy initial data
    HRESULT hr = resources.bandingCB->Map(0, nullptr, reinterpret_cast<void**>(&resources.bandingCBStart));
    Utils::Validate(hr, L"Error: failed to map the banding constant buffer!");

    memcpy(resources.bandingCBStart, &constants, sizeof(BandingConstants));
}

/**
 * Copy a texture from the CPU to the GPU upload heap, then schedule a copy to the default heap.
 */
void Upload_Texture(D3D12Global &d3d, ID3D12Resource* destResource, ID3D12Resource* srcResource, const TextureInfo &texture, UINT subresourceIndex)
{
    // Copy the pixel data to the upload heap resource
    UINT8* pData;
    HRESULT hr = srcResource->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    memcpy(pData + texture.offset, texture.pixels.data(), texture.width * texture.height * texture.stride);
    srcResource->Unmap(0, nullptr);

    // Describe the upload heap resource location for the copy
    D3D12_SUBRESOURCE_FOOTPRINT subresource = {};
    subresource.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    subresource.Width = texture.width;
    subresource.Height = texture.height;
    subresource.RowPitch = (texture.width * texture.stride);
    subresource.Depth = 1;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Offset = texture.offset;
    footprint.Footprint = subresource;

    D3D12_TEXTURE_COPY_LOCATION source = {};
    source.pResource = srcResource;
    source.PlacedFootprint = footprint;
    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    // Describe the default heap resource location for the copy
    D3D12_TEXTURE_COPY_LOCATION destination = {};
    destination.pResource = destResource;
    destination.SubresourceIndex = subresourceIndex;
    destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    // Copy the buffer resource from the upload heap to the texture resource on the default heap
    d3d.cmdList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
}

/**
* Load a vertex and pixel shader.
*/
void Load_Shaders(D3D12Resources &resources, D3D12ShaderCompilerInfo &shaderCompiler)
{
    D3D12ShaderInfo vsInfo = D3D12ShaderInfo(L"shaders/ColorBanding.hlsl", L"VS", L"vs_6_0");
    D3DShaders::Compile_Shader(shaderCompiler, vsInfo, &resources.vsBytecode);

    D3D12ShaderInfo psInfo = D3D12ShaderInfo(L"shaders/ColorBanding.hlsl", L"PS", L"ps_6_0");
    D3DShaders::Compile_Shader(shaderCompiler, psInfo, &resources.psBytecode);
}

/**
* Loads a set of blue noise textures from disk and uploads them to the GPU.
*/
void Load_Blue_Noise_Texture_Array(D3D12Global &d3d, D3D12Resources &resources, UINT num)
{
    TextureInfo* textures = new TextureInfo[num];
    for (UINT i = 0; i < num; i++)
    {
        string filepath = "data\\blue-noise\\LDR_RGB1_";
        filepath.append(to_string(i));
        filepath.append(".png");
        textures[i] = Utils::LoadTexture(filepath);
        if (i > 0) textures[i].offset = i * textures[i - 1].width * textures[i - 1].height * textures[i - 1].stride;
    }

    // Describe the texture array
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Width = textures[0].width;
    textureDesc.Height = textures[0].height;
    textureDesc.MipLevels = 1;
    textureDesc.DepthOrArraySize = num;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Create the texture resource on the default heap
    HRESULT hr = d3d.device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resources.blueNoiseArray));
    Utils::Validate(hr, L"Error: failed to create texture resource (default heap)!");
#if NAME_D3D_RESOURCES
    resources.blueNoiseArray->SetName(L"Blue Noise");
#endif

    // Create the SRV on the descriptor heap
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.ArraySize = num;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = resources.descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += d3d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    d3d.device->CreateShaderResourceView(resources.blueNoiseArray, &srvDesc, handle);

    // Describe the upload resource
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Width = (textures[0].width * textures[0].height * textures[0].stride) * num;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

    // Create the buffer resource on the upload heap
    hr = d3d.device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resources.blueNoiseArrayUploadResource));
    Utils::Validate(hr, L"Error: failed to create buffer resource (upload heap)!");
#if NAME_D3D_RESOURCES
    resources.blueNoiseArrayUploadResource->SetName(L"Blue Noise Array Upload Buffer");
#endif

    // Upload the textures to the GPU
    for (UINT i = 0; i < num; i++)
    {
        Upload_Texture(d3d, resources.blueNoiseArray, resources.blueNoiseArrayUploadResource, textures[i], i);
    }

    // Transition the texture to a shader resource
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resources.blueNoiseArray;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    d3d.cmdList->ResourceBarrier(1, &barrier);

    SAFE_DELETE_ARRAY(textures);
}

/**
 * Load a blue noise texture from disk and uploads it to the GPU.
 */
void Load_Blue_Noise_Texture(D3D12Global &d3d, D3D12Resources &resources)
{
    string filepath = "data\\blue-noise\\rgb-256.png";
    TextureInfo texture = Utils::LoadTexture(filepath);

    // Describe the texture
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Width = texture.width;
    textureDesc.Height = texture.height;
    textureDesc.MipLevels = 1;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Create the texture resource on the default heap
    HRESULT hr = d3d.device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resources.blueNoise));
    Utils::Validate(hr, L"Error: failed to create texture resource (default heap)!");
#if NAME_D3D_RESOURCES
    resources.blueNoise->SetName(L"Blue Noise");
#endif

    // Create the SRV on the descriptor heap
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.ArraySize = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    D3D12_CPU_DESCRIPTOR_HANDLE handle = resources.descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    d3d.device->CreateShaderResourceView(resources.blueNoise, &srvDesc, handle);

    // Describe the upload resource
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Width = texture.width * texture.height * texture.stride;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

    // Create the buffer resource on the upload heap
    hr = d3d.device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resources.blueNoiseUploadResource));
    Utils::Validate(hr, L"Error: failed to create buffer resource (upload heap)!");
#if NAME_D3D_RESOURCES
    resources.blueNoiseUploadResource->SetName(L"Blue Noise Upload Buffer");
#endif

    // Upload the texture to the GPU
    Upload_Texture(d3d, resources.blueNoise, resources.blueNoiseUploadResource, texture, 0);

    // Transition the texture to a shader resource
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resources.blueNoise;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    d3d.cmdList->ResourceBarrier(1, &barrier);
}

/**
* Release the resources.
*/
void Destroy(D3D12Resources &resources)
{
    if (resources.bandingCB) resources.bandingCB->Unmap(0, nullptr);

    SAFE_RELEASE(resources.bandingCB);
    SAFE_RELEASE(resources.blueNoise);
    SAFE_RELEASE(resources.blueNoiseUploadResource);
    SAFE_RELEASE(resources.blueNoiseArray);
    SAFE_RELEASE(resources.blueNoiseArrayUploadResource);
    SAFE_RELEASE(resources.rtvHeap);
    SAFE_RELEASE(resources.descriptorHeap);
    SAFE_RELEASE(resources.uiDescriptorHeap);
    SAFE_RELEASE(resources.rs);
    SAFE_RELEASE(resources.pso);
    SAFE_RELEASE(resources.vsBytecode);
    SAFE_RELEASE(resources.psBytecode);
}

}

//--------------------------------------------------------------------------------------
// D3D12 Shader Functions
//--------------------------------------------------------------------------------------

namespace D3DShaders
{

/**
* Compile an HLSL shader using dxcompiler.
*/
void Compile_Shader(D3D12ShaderCompilerInfo &compilerInfo, D3D12ShaderInfo &info, IDxcBlob** blob)
{
    UINT32 code(0);
    IDxcBlobEncoding* pShaderText(nullptr);

    // Load and encode the shader file
    HRESULT hr = compilerInfo.library->CreateBlobFromFile(info.filename, &code, &pShaderText);
    Utils::Validate(hr, L"Error: failed to create blob from shader file!");

    // Create the compiler include handler
    CComPtr<IDxcIncludeHandler> dxcIncludeHandler;
    hr = compilerInfo.library->CreateIncludeHandler(&dxcIncludeHandler);
    Utils::Validate(hr, L"Error: failed to create include handler");

    // Compile the shader
    IDxcOperationResult* result;
    hr = compilerInfo.compiler->Compile(
        pShaderText,
        info.filename,
        info.entryPoint,
        info.targetProfile,
        info.arguments,
        info.argCount,
        info.defines,
        info.defineCount,
        dxcIncludeHandler,
        &result);

    Utils::Validate(hr, L"Error: failed to compile shader!");

    // Verify the result
    result->GetStatus(&hr);
    if (FAILED(hr)) 
    {
        IDxcBlobEncoding* error;
        hr = result->GetErrorBuffer(&error);
        Utils::Validate(hr, L"Error: failed to get shader compiler error buffer!");

        // Convert error blob to a string
        vector<char> infoLog(error->GetBufferSize() + 1);
        memcpy(infoLog.data(), error->GetBufferPointer(), error->GetBufferSize());
        infoLog[error->GetBufferSize()] = 0;

        string errorMsg = "Shader Compiler Error:\n";
        errorMsg.append(infoLog.data());

        MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
        return;
    }

    hr = result->GetResult(blob);
    Utils::Validate(hr, L"Error: failed to get shader blob result!");
}

/**
* Initialize the shader compiler.
*/
void Init_Shader_Compiler(D3D12ShaderCompilerInfo &shaderCompiler)
{
    HRESULT hr = shaderCompiler.DxcDllHelper.Initialize();
    Utils::Validate(hr, L"Failed to initialize DxCDllSupport!");

    hr = shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcCompiler, &shaderCompiler.compiler);
    Utils::Validate(hr, L"Failed to create DxcCompiler!");

    hr = shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcLibrary, &shaderCompiler.library);
    Utils::Validate(hr, L"Failed to create DxcLibrary!");
}

/**
* Release shader compiler resources.
*/
void Destroy(D3D12ShaderCompilerInfo &shaderCompiler)
{
    SAFE_RELEASE(shaderCompiler.compiler);
    SAFE_RELEASE(shaderCompiler.library);
    shaderCompiler.DxcDllHelper.Cleanup();
}

}

//--------------------------------------------------------------------------------------
// D3D12 Functions
//--------------------------------------------------------------------------------------

namespace D3D12
{

/**
* Create the device.
*/
void Create_Device(D3D12Global &d3d)
{
#if _DEBUG
    // Enable the D3D12 debug layer.
    {
        ID3D12Debug* debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    // Create a DXGI Factory
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d.factory));
    Utils::Validate(hr, L"Error: failed to create DXGI factory!");

    // Create the device
    d3d.adapter = nullptr;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != d3d.factory->EnumAdapters1(adapterIndex, &d3d.adapter); adapterIndex++)
    {
        DXGI_ADAPTER_DESC1 adapterDesc;
        d3d.adapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {            
            continue;    // Don't select the Basic Render Driver adapter.
        }

        if (SUCCEEDED(D3D12CreateDevice(d3d.adapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device5), (void**)&d3d.device)))
        {
#if NAME_D3D_RESOURCES
            d3d.device->SetName(L"D3D12 Device");
#endif
            break;
        }

        if (d3d.device == nullptr)
        {
            // Didn't find a D3D12 device
            Utils::Validate(E_FAIL, L"Error: failed to create a D3D12 device!");
        }
    }
}

/**
* Create the command queue.
*/
void Create_Command_Queue(D3D12Global &d3d)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    HRESULT hr = d3d.device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d.cmdQueue));
    Utils::Validate(hr, L"Error: failed to create command queue!");
#if NAME_D3D_RESOURCES
    d3d.cmdQueue->SetName(L"D3D12 Command Queue");
#endif
}

/**
* Create the command allocator for each frame.
*/
void Create_Command_Allocator(D3D12Global &d3d)
{
    for (UINT n = 0; n < 2; n++)
    {
        HRESULT hr = d3d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d.cmdAlloc[n]));
        Utils::Validate(hr, L"Error: failed to create the command allocator!");
#if NAME_D3D_RESOURCES
        if(n == 0) d3d.cmdAlloc[n]->SetName(L"D3D12 Command Allocator 0");
        else d3d.cmdAlloc[n]->SetName(L"D3D12 Command Allocator 1");
#endif
    }
}

/**
* Create the command list.
*/
void Create_CommandList(D3D12Global &d3d)
{
    HRESULT hr = d3d.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d.cmdAlloc[d3d.frameIndex], nullptr, IID_PPV_ARGS(&d3d.cmdList));
    hr = d3d.cmdList->Close();
    Utils::Validate(hr, L"Error: failed to create the command list!");
#if NAME_D3D_RESOURCES
    d3d.cmdList->SetName(L"D3D12 Command List");
#endif
}

/**
* Create a fence.
*/
void Create_Fence(D3D12Global &d3d)
{
    HRESULT hr = d3d.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d.fence));
    Utils::Validate(hr, L"Error: failed to create fence!");
#if NAME_D3D_RESOURCES
    d3d.fence->SetName(L"D3D12 Fence");
#endif

    d3d.fenceValues[d3d.frameIndex]++;

    // Create an event handle to use for frame synchronization
    d3d.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    if (d3d.fenceEvent == nullptr)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Utils::Validate(hr, L"Error: failed to create fence event!");
    }
}

/**
* Create the viewport.
*/
void Create_Viewport(D3D12Global &d3d)
{
    d3d.viewport.Width = (float)d3d.width;
    d3d.viewport.Height = (float)d3d.height;
    d3d.viewport.MinDepth = D3D12_MIN_DEPTH;
    d3d.viewport.MaxDepth = D3D12_MAX_DEPTH;
    d3d.viewport.TopLeftX = 0.f;
    d3d.viewport.TopLeftY = 0.f;
}

/**
 * Create the scissor.
 */
void Create_Scissor(D3D12Global &d3d)
{
    d3d.scissor.left = 0;
    d3d.scissor.top = 0;
    d3d.scissor.right = d3d.width;
    d3d.scissor.bottom = d3d.height;
}

/**
* Create the swap chain.
*/
void Create_SwapChain(D3D12Global &d3d, HWND &window)
{
    // Describe the swap chain
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.BufferCount = 2;
    desc.Width = d3d.width;
    desc.Height = d3d.height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    // Create the swap chain
    IDXGISwapChain1* swapChain;
    HRESULT hr = d3d.factory->CreateSwapChainForHwnd(d3d.cmdQueue, window, &desc, nullptr, nullptr, &swapChain);
    Utils::Validate(hr, L"Error: failed to create swap chain!");

    // Associate the swap chain with a window
    hr = d3d.factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
    Utils::Validate(hr, L"Error: failed to make window association!");

    // Get the swap chain interface
    hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&d3d.swapChain));
    Utils::Validate(hr, L"Error: failed to cast swap chain!");

    SAFE_RELEASE(swapChain);
    d3d.frameIndex = d3d.swapChain->GetCurrentBackBufferIndex();
}

/**
* Create a root signature.
*/
ID3D12RootSignature* Create_Root_Signature(D3D12Global &d3d, const D3D12_ROOT_SIGNATURE_DESC &desc)
{
    ID3DBlob* sig;
    ID3DBlob* error;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error);
    Utils::Validate(hr, L"Error: failed to serialize root signature!");

    ID3D12RootSignature* pRootSig;
    hr = d3d.device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
    Utils::Validate(hr, L"Error: failed to create root signature!");

    SAFE_RELEASE(sig);
    SAFE_RELEASE(error);
    return pRootSig;
}

/**
 * Run a fullscreen graphics pass.
 */
void Build_CmdList(D3D12Global &d3d, D3D12Resources &resources)
{
    // Transition the back buffer to a render target
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Transition.pResource = d3d.backBuffer[d3d.frameIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // Wait for the transition to complete
    d3d.cmdList->ResourceBarrier(1, &barrier);

    // Set the render target
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = resources.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += (resources.rtvDescSize * d3d.frameIndex);
    d3d.cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    // Set the descriptor heaps
    ID3D12DescriptorHeap* ppHeaps[] = { resources.descriptorHeap };
    d3d.cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // Set root signature, parameters, and pipeline state
    d3d.cmdList->SetGraphicsRootSignature(resources.rs);
    d3d.cmdList->SetGraphicsRootConstantBufferView(0, resources.bandingCB->GetGPUVirtualAddress());
    d3d.cmdList->SetGraphicsRootDescriptorTable(1, resources.descriptorHeap->GetGPUDescriptorHandleForHeapStart());
    d3d.cmdList->SetPipelineState(resources.pso);

    // Set necessary state
    d3d.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d.cmdList->RSSetViewports(1, &d3d.viewport);
    d3d.cmdList->RSSetScissorRects(1, &d3d.scissor);

    // Draw
    d3d.cmdList->DrawInstanced(3, 1, 0, 0);

    // Transition the back buffer to present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    // Wait for the transition to complete
    d3d.cmdList->ResourceBarrier(1, &barrier);
}

/**
* Reset the command list.
*/
void Reset_CommandList(D3D12Global &d3d)
{
    // Reset the command allocator for the current frame
    HRESULT hr = d3d.cmdAlloc[d3d.frameIndex]->Reset();
    Utils::Validate(hr, L"Error: failed to reset the D3D command allocator!");

    // Reset the ray tracing command list for the current frame
    hr = d3d.cmdList->Reset(d3d.cmdAlloc[d3d.frameIndex], nullptr);
    Utils::Validate(hr, L"Error: failed to reset the D3D command list!");
}

/*
* Submit the command list.
*/
void Submit_CmdList(D3D12Global &d3d)
{
    d3d.cmdList->Close();

    ID3D12CommandList* pGraphicsList = { d3d.cmdList };
    d3d.cmdQueue->ExecuteCommandLists(1, &pGraphicsList);
    d3d.fenceValues[d3d.frameIndex]++;
    d3d.cmdQueue->Signal(d3d.fence, d3d.fenceValues[d3d.frameIndex]);
}

/**
* Swap the back buffers.
*/
void Present(D3D12Global &d3d)
{
    HRESULT hr = d3d.swapChain->Present(d3d.vsync, 0);
    if (FAILED(hr))
    {
        hr = d3d.device->GetDeviceRemovedReason();
        Utils::Validate(hr, L"Error: failed to present!");
    }
}

/*
* Wait for pending GPU work to complete.
*/
void WaitForGPU(D3D12Global &d3d)
{
    // Schedule a signal command in the queue
    HRESULT hr = d3d.cmdQueue->Signal(d3d.fence, d3d.fenceValues[d3d.frameIndex]);
    Utils::Validate(hr, L"Error: failed to signal fence!");

    // Wait until the fence has been processed
    hr = d3d.fence->SetEventOnCompletion(d3d.fenceValues[d3d.frameIndex], d3d.fenceEvent);
    Utils::Validate(hr, L"Error: failed to set fence event!");

    WaitForSingleObjectEx(d3d.fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame
    d3d.fenceValues[d3d.frameIndex]++;
}

/**
* Prepare to render the next frame.
*/
void MoveToNextFrame(D3D12Global &d3d)
{
    // Schedule a Signal command in the queue
    const UINT64 currentFenceValue = d3d.fenceValues[d3d.frameIndex];
    HRESULT hr = d3d.cmdQueue->Signal(d3d.fence, currentFenceValue);
    Utils::Validate(hr, L"Error: failed to signal command queue!");

    // Update the frame index
    d3d.frameIndex = d3d.swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is
    if (d3d.fence->GetCompletedValue() < d3d.fenceValues[d3d.frameIndex])
    {
        hr = d3d.fence->SetEventOnCompletion(d3d.fenceValues[d3d.frameIndex], d3d.fenceEvent);
        Utils::Validate(hr, L"Error: failed to set fence value!");

        WaitForSingleObjectEx(d3d.fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame
    d3d.fenceValues[d3d.frameIndex] = currentFenceValue + 1;
}

/**
* Release D3D12 resources.
*/
void Destroy(D3D12Global &d3d)
{
    SAFE_RELEASE(d3d.fence);
    SAFE_RELEASE(d3d.backBuffer[1]);
    SAFE_RELEASE(d3d.backBuffer[0]);
    SAFE_RELEASE(d3d.swapChain);
    SAFE_RELEASE(d3d.cmdAlloc[0]);
    SAFE_RELEASE(d3d.cmdAlloc[1]);
    SAFE_RELEASE(d3d.cmdQueue);
    SAFE_RELEASE(d3d.cmdList);
    SAFE_RELEASE(d3d.device);
    SAFE_RELEASE(d3d.adapter);
    SAFE_RELEASE(d3d.factory);
}

}
