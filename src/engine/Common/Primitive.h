#pragma once
#include"Vec.h"
class Primitive
{
public:
	enum TYPE { SPHERE, PLANE, TRIANGLE, RAY };
	virtual TYPE GetPrimitiveType() = 0;
};

// 球
class Sphere : public Primitive
{
public:
	// 中心座標
	Vec3<float> center = {};
	// 半径
	float radius = 1.0f;

	TYPE GetPrimitiveType()override { return SPHERE; }
};

//平面
class Plane : public Primitive
{
public:
	// 法線ベクトル
	Vec3<float> normal = { 0,1,0 };
	// 原点(0,0,0)からの距離
	float distance = 0.0f;

	TYPE GetPrimitiveType()override { return PLANE; }
};

// 法線付き三角形（時計回りが表面）
class Triangle : public Primitive
{
public:
	// 頂点座標3つ
	Vec3<float>	p0;
	Vec3<float>	p1;
	Vec3<float>	p2;
	// 法線ベクトル
	Vec3<float>	normal;

	// 法線の計算
	void ComputeNormal()
	{
		Vec3<float> p0_p1 = p1 - p0;
		Vec3<float> p0_p2 = p2 - p0;

		// 外積により、2辺に垂直なベクトルを算出する
		normal = p0_p1.Cross(p0_p2).GetNormal();
	}

	TYPE GetPrimitiveType()override { return TRIANGLE; }
};

class Ray : public Primitive
{
public:
	// 始点座標
	Vec3<float>	start = { 0,0,0 };
	// 方向
	Vec3<float>	dir = { 1,0,0 };

	TYPE GetPrimitiveType()override { return RAY; }
};