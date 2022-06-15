#pragma once
#include"Vec.h"
#include<vector>
#include"Transform.h"

class CollisionPrimitive
{
public:
	enum SHAPE { SPHERE, MESH };

private:
	const SHAPE shape;
	
protected:
	CollisionPrimitive() = delete;
	CollisionPrimitive(CollisionPrimitive&& arg) = delete;
	CollisionPrimitive(const CollisionPrimitive& arg) = delete;
	CollisionPrimitive(const SHAPE& Shape) :shape(Shape) {}

public:
	const SHAPE& GetShape()const { return shape; }
};

//球
class CollisionSphere : public CollisionPrimitive
{
	friend class Collision;
	Transform* world = nullptr;	//ワールドトランスフォーム

public:
	Vec3<float>localCenter;	//中心（ローカル座標）
	float radius;					//半径
	CollisionSphere(const float& Radius, const Vec3<float>& CenterOffset = Vec3<float>(0, 0, 0))
		:CollisionPrimitive(SPHERE), localCenter(CenterOffset), radius(Radius) {}

	//ゲッタ
	const Matrix& GetWorldMat()
	{ 
		if (!world)return XMMatrixIdentity();
		return world->GetMat();
	}
	
	//セッタ
	void AttachWorldTransform(Transform* World) { world = World; }
};

//法線つき三角形（ローカル座標）
struct CollisionTriangle
{
	Vec3<float>p0;
	Vec3<float>p1;
	Vec3<float>p2;
	Vec3<float>normal;

	//法線の計算
	void CalculateNormal()
	{
		Vec3<float>p0_p1 = p1 - p0;
		Vec3<float>p0_p2 = p2 - p0;

		//外積により、２辺に垂直なベクトルを算出
		normal = p0_p1.Cross(p0_p2).GetNormal();
	}
};

class CollisionMesh : public CollisionPrimitive
{
	friend class Collision;

	//三角メッシュ配列
	std::vector<CollisionTriangle>triangles;

	//ワールドトランスフォーム
	Transform* world = nullptr;

public:
	CollisionMesh(const std::vector<CollisionTriangle>& Triangles, Transform* World = nullptr) 
		: CollisionPrimitive(MESH)
	{
		SetTriangles(Triangles);
		AttachWorldTransform(World); 
	}

	//ゲッタ
	const std::vector<CollisionTriangle>& GetTriangles()const { return triangles; }
	const Matrix& GetWorldMat()
	{
		if (!world)return XMMatrixIdentity();
		return world->GetMat();
	}

	//セッタ
	void SetTriangles(const std::vector<CollisionTriangle>& Triangles) { triangles = Triangles; }
	void AttachWorldTransform(Transform* World) { world = World; }
};

static class Collision
{
	static bool SphereAndSphere(CollisionSphere* SphereA, CollisionSphere* SphereB, Vec3<float>* Inter);

	static Vec3<float> ClosestPtPoint2Triangle(const Vec3<float>& Pt, const CollisionTriangle& Tri, const Matrix& MeshWorld);
	static bool SphereAndMesh(CollisionSphere* Sphere, CollisionMesh* Mesh, Vec3<float>* Inter);
public:
	static bool CheckPrimitiveHit(CollisionPrimitive* PrimitiveA, CollisionPrimitive* PrimitiveB, Vec3<float>* Inter);
};