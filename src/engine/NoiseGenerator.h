#pragma once
#include<memory>
#include"Vec.h"
class TextureBuffer;
class NoiseGenerator
{
	static int PERLIN_NOISE_ID;
public:
	static void CountReset()
	{
		PERLIN_NOISE_ID = 0;
	}
	static void PerlinNoise(std::shared_ptr<TextureBuffer>DestTex, const int& Split, const int& Octaves = 1, const float& Frequency = 1.0f, const float& Persistance = 0.5f);
	static std::shared_ptr<TextureBuffer>PerlinNoise(const Vec2<int>& Size, const int& Split, const int& Octaves = 1, const float& Frequency = 1.0f, const float& Persistance = 0.5f);
};

