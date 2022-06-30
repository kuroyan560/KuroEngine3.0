#pragma once
#include<memory>
#include"Vec.h"
class TextureBuffer;
class NoiseGenerator
{
public:
	static std::shared_ptr<TextureBuffer>PerlinNoise(const Vec2<int>& Size, const int& Split, const int& Octaves = 1, const float& Persistence = 0.5f);
};

