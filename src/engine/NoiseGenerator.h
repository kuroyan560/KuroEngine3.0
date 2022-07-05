#pragma once
#include<memory>
#include"Vec.h"
#include"D3D12Data.h"
class NoiseGenerator
{
	static int PERLIN_NOISE_ID_2D;
public:
	static void CountReset()
	{
		PERLIN_NOISE_ID_2D = 0;
	}
	static void PerlinNoise2D(std::shared_ptr<TextureBuffer>DestTex, const int& Split, const int& Octaves = 1, const float& Frequency = 1.0f, const float& Persistance = 0.5f);
	static std::shared_ptr<TextureBuffer>PerlinNoise2D(const Vec2<int>& Size, const int& Split, const int& Octaves = 1, const float& Frequency = 1.0f, const float& Persistance = 0.5f, const DXGI_FORMAT& Format = DXGI_FORMAT_R32G32_FLOAT);
};

