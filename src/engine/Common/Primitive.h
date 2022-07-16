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
	Vec3<float> m_center = {};
	// 半径
	float m_radius = 1.0f;

	TYPE GetPrimitiveType()override { return SPHERE; }
};

//平面
class Plane : public Primitive
{
public:
	// 法線ベクトル
	Vec3<float> m_normal = { 0,1,0 };
	// 原点(0,0,0)からの距離
	float m_distance = 0.0f;

	TYPE GetPrimitiveType()override { return PLANE; }
};

// 法線付き三角形（時計回りが表面）
class Triangle : public Primitive
{
public:
	// 頂点座標3つ
	Vec3<float>	m_p0;
	Vec3<float>	m_p1;
	Vec3<float>	m_p2;
	// 法線ベクトル
	Vec3<float>	m_normal;

	// 法線の計算
	void ComputeNormal()
	{
		Vec3<float> p0_p1 = m_p1 - m_p0;
		Vec3<float> p0_p2 = m_p2 - m_p0;

		// 外積により、2辺に垂直なベクトルを算出する
		m_normal = p0_p1.Cross(p0_p2).GetNormal();
	}

	TYPE GetPrimitiveType()override { return TRIANGLE; }
};

class Ray : public Primitive
{
public:
	// 始点座標
	Vec3<float>	m_start = { 0,0,0 };
	// 方向
	Vec3<float>	m_dir = { 1,0,0 };

	TYPE GetPrimitiveType()override { return RAY; }
};