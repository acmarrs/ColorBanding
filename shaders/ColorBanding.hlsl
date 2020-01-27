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

// ---[ Structures ]---

struct PSInput
{
	float4 position : SV_POSITION;
	float2 tex0		: TEXCOORD;
};

// ---[ Resources ]---

cbuffer BandingConstants : register(b0)
{
	float3	lightPosition;
	float	noiseScale;
	float3	color;
	uint	resolutionX;
	uint	frameNumber;
	int		useNoise;
	int		showNoise;
	int		noiseType;
};

Texture2DArray<float4> blueNoise : register(t0);

// ---[ Vertex Shader ]---

PSInput VS(uint VertID : SV_VertexID)
{
	float div2 = (VertID / 2);
	float mod2 = (VertID % 2);

	PSInput result;
	result.position.x = (mod2 * 4.f) - 1.f;
	result.position.y = (div2 * 4.f) - 1.f;
	result.position.zw = float2(0.f, 1.f);

	result.tex0.x = mod2 * 2.f;
	result.tex0.y = 1.f - (div2 * 2.f);

	return result;
}

// ---[ Random Number Generation ]---

/*
 * From Nathan Reed's blog at:
 * http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
*/

uint WangHash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

uint Xorshift(uint seed)
{
	// Xorshift algorithm from George Marsaglia's paper
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);
	return seed;
}

float GenerateRandomNumber(inout uint seed)
{
	seed = WangHash(seed);
	return float(Xorshift(seed)) * (1.f / 4294967296.f);
}

/**
* Generate three components of white noise.
*/
float3 GetWhiteNoise(uint2 screenPosition, uint screenWidth, uint frame)
{
	// Generate a unique seed on screen position and time
	uint seed = ((screenPosition.y * screenWidth) + screenPosition.x) * frame;

	// Generate uniformly distributed random values in the range [0, 1]
	float3 rnd0;
	rnd0.x = GenerateRandomNumber(seed);
	rnd0.y = GenerateRandomNumber(seed);
	rnd0.z = GenerateRandomNumber(seed);

	// Shift the random value into the range [-1, 1]
	rnd0 = mad(rnd0, 2.f, - 1.f);

	// Scale the noise magnitude to [-noiseScale, noiseScale]
	return (rnd0 * noiseScale);
}

/**
* Generate three components of blue noise.
* Blue noise textures from Christoph Peters at: http://momentsingraphics.de/BlueNoise.html
*/
float3 GetBlueNoise(uint2 screenPosition, uint screenWidth, uint frame)
{
	// Generate a unique seed based on screen position and time
	uint seed = ((screenPosition.y * screenWidth) + screenPosition.x) * frame;
	
	// Choose a random blue noise slice in the texture array based on the seed
	uint z = GenerateRandomNumber(seed) * 63;

	// Load a blue noise value
	float3 rnd0 = blueNoise.Load(int4(screenPosition.xy % 64, z, 0)).rgb;
	
	// Transform a uniform distribution on [0, 1] to a symmetric triangular distribution on [-1, 1]
	rnd0 = mad(rnd0, 2.f, -1.f);
	rnd0 = sign(rnd0) * (1.f - sqrt((1.f - abs(rnd0))));

	// Scale the noise magnitude to [-noiseScale, noiseScale]
	return (rnd0 * noiseScale);
}

// ---[ Pixel Shader ]---

float4 PS(PSInput input) : SV_TARGET
{
	float3 worldPosition = float3(input.position.x, 0.f, input.position.y);
	float3 normal = float3(0.f, 1.f, 0.f);
	float3 lightVector = float3(lightPosition - worldPosition);
	float3 lightDirection = normalize(lightVector);

	// Compute color
	float3 result = saturate(color  * dot(normal, lightDirection));
	
	if (useNoise > 0)
	{
		// Compute the noise
		float3 noise = 0;
		if (noiseType == 0)
		{
			noise = GetWhiteNoise(uint2(input.position.xy), resolutionX, frameNumber);
		}
		else
		{
			noise =  GetBlueNoise(uint2(input.position.xy), resolutionX, frameNumber);
		}

		if (showNoise)
		{
			return float4(noise, 1.f);
		}

		// Add the noise to the color
		result += noise;
	}

	// Gamma adjust
	result = sqrt(result);
	return float4(result, 1.f);
}
