#pragma once
#include<memory>
#include"Vec.h"
class TextureBuffer;
class NoiseGenerator
{
private:
	static float Wavelet(float t);
public:
	static std::shared_ptr<TextureBuffer>PerlinNoise(const Vec2<int>& Size, const int& Split);
	static std::shared_ptr<TextureBuffer>PerlinNoiseFractal(const Vec2<int>& Size, const int& Split, const int& Octaves, const float& Persistence = 0.5f);
};

