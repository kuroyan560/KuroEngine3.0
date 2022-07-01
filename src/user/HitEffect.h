#pragma once
#include"Vec.h"
#include<array>

#include<memory>
class TextureBuffer;

class HitEffect
{
private:
	static const int MAX_NUM = 300;
	static std::array<HitEffect, MAX_NUM>INSTANCES;

public:
	HitEffect() {}

	static void Generate(const Vec2<float>& Pos);
	static void Init();
	static void Draw(std::shared_ptr<TextureBuffer>&Noise, std::shared_ptr<TextureBuffer>& Noise2);
	static HitEffect& GetInstance(const int& Idx) { return INSTANCES[Idx]; }

public:
	char isActive = 0;
	Vec2<float>pos = { 0,0 };
	float blur = 0.0f;
	float scale = 1.0f;
	float uvOffsetAmount = 0.0f;
};
