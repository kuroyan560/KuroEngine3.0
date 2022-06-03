#pragma once
#include"Vec.h"
#include<vector>

struct Color
{
	//0 ~ 255
	static Vec4<float>ConvertToVec4(int R, int G, int B, int A)
	{
		auto color = Color(R, G, B, A);
		return Vec4<float>(color.r, color.g, color.b, color.a);
	}

public:
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
	float a = 1.0f;

	//コンストラクタ
	Color() {};
	Color(const Color& Color) :r(Color.r), g(Color.g), b(Color.b), a(Color.a) {}
	Color(float R, float G, float B, float A) : r(R), g(G), b(B), a(A) {}
	Color(int R, int G, int B, int A) {
		r = R / 255.0f;
		g = G / 255.0f;
		b = B / 255.0f;
		a = A / 255.0f;
	}

	//比較演算子
	bool operator==(const Color& rhs) {
		if (r != rhs.r)return false;
		if (g != rhs.g)return false;
		if (b != rhs.b)return false;
		if (a != rhs.a)return false;
		return true;
	}
	bool operator!=(const Color& rhs) {
		return !(*this == rhs);
	}

	//代入演算子
	void operator=(const Color& rhs)
	{
		r = rhs.r;
		g = rhs.g;
		b = rhs.b;
		a = rhs.a;
	}
};