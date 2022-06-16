#pragma once
#include"Vec.h"
#include<vector>
#include"Transform.h"
#include<memory>
#include"D3D12Data.h"
class Camera;
class CollisionPrimitive
{
public:
	enum SHAPE { SPHERE, MESH };

private:
	friend class Collider;
	const SHAPE shape;
	
protected:
	//定数バッファ用データ
	struct ConstData
	{
		Matrix world = XMMatrixIdentity();
		unsigned int hit = 0;
	};

	CollisionPrimitive() = delete;
	CollisionPrimitive(CollisionPrimitive&& arg) = delete;
	CollisionPrimitive(const CollisionPrimitive& arg) = delete;
	CollisionPrimitive(const SHAPE& Shape) :shape(Shape) {}
	virtual void DebugDraw(const bool& Hit, Camera& Cam) = 0;	//当たり判定の可視化

public:
	const SHAPE& GetShape()const { return shape; }
};

//球
class CollisionSphere : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	Transform* world = nullptr;	//ワールドトランスフォーム
	std::shared_ptr<ConstantBuffer>constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;
	
public:
	Vec3<float>localCenter;	//中心（ローカル座標）
	float radius;					//半径
	CollisionSphere(const float& Radius, Transform* World = nullptr, const Vec3<float>& CenterOffset = Vec3<float>(0, 0, 0))
		:CollisionPrimitive(SPHERE), world(World), localCenter(CenterOffset), radius(Radius) {}

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
private:
	friend class Collision;

private:
	//頂点バッファ
	std::shared_ptr<VertexBuffer>vertBuff;

	//定数バッファ
	std::shared_ptr<ConstantBuffer>constBuff;

	//三角メッシュ配列
	std::vector<CollisionTriangle>triangles;

	//ワールドトランスフォーム
	Transform* world = nullptr;
	void DebugDraw(const bool& Hit, Camera& Cam)override;

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
	void SetTriangles(const std::vector<CollisionTriangle>& Triangles);
	void AttachWorldTransform(Transform* World) { world = World; }
};

class Collision
{
	static bool SphereAndSphere(CollisionSphere* SphereA, CollisionSphere* SphereB, Vec3<float>* Inter);

	static Vec3<float> ClosestPtPoint2Triangle(const Vec3<float>& Pt, const CollisionTriangle& Tri, const Matrix& MeshWorld);
	static bool SphereAndMesh(CollisionSphere* Sphere, CollisionMesh* Mesh, Vec3<float>* Inter);
public:
	static bool CheckPrimitiveHit(CollisionPrimitive* PrimitiveA, CollisionPrimitive* PrimitiveB, Vec3<float>* Inter);
};