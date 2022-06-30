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
	static void Draw(/*std::shared_ptr<TextureBuffer>&Noise*/);

private:
	char isActive = 0;
	Vec2<float>pos = { 0,0 };
};
