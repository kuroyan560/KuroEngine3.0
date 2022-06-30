#pragma once
#include"Vec.h"
#include<array>
class HitEffect
{
private:
	static const int MAX_NUM = 300;
	static std::array<HitEffect, MAX_NUM>INSTANCES;

public:
	static void Generate(const Vec2<float>& Pos);
	static void Init();
	static void Draw();

private:
	char isActive = 0;
	Vec2<float>pos = { 0,0 };
};
