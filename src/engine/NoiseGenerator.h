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
};

