#pragma once
#include"Vec.h"
#include"ValueMinMax.h"
#include<vector>
#include"Transform.h"
#include<memory>
#include"D3D12Data.h"
class Camera;
class CollisionPrimitive
{
public:
	enum SHAPE { SPHERE, AABB, MESH };

private:
	friend class Collider;
	const SHAPE shape;
	
protected:
	//基本的なプリミティブ当たり判定のデバッグ描画
	static std::shared_ptr<GraphicsPipeline>GetPrimitivePipeline();
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

//AABB(軸並行境界ボックス）
class CollisionAABB : public CollisionPrimitive
{
private:
	friend class Collision;
	enum VERT_IDX {
		LU_NZ, RU_NZ, RB_NZ, LB_NZ,	//手前
		LU_FZ, RU_FZ, RB_FZ, LB_FZ,	//奥
		VERT_NUM
	};

private:
	Transform* world = nullptr;	//ワールドトランスフォーム
	std::shared_ptr<ConstantBuffer>constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>vertBuff;
	//各軸の最小値と最大値
	Vec3<ValueMinMax>pValues;

public:
	CollisionAABB(const Vec3<ValueMinMax>& PValues, Transform* World = nullptr)
		:CollisionPrimitive(AABB), world(World) { StructBox(PValues);	}

	//ゲッタ
	const Vec3<ValueMinMax>& GetPtValue() { return pValues; }
	const Matrix& GetWorldMat()
	{
		if (!world)return XMMatrixIdentity();
		return world->GetMat();
	}
	//セッタ
	void StructBox(const Vec3<ValueMinMax>& PValues);
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
	std::shared_ptr<VertexBuffer>vertBuff;
	std::shared_ptr<ConstantBuffer>constBuff;
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
	//球と球
	static bool SphereAndSphere(CollisionSphere* SphereA, CollisionSphere* SphereB, Vec3<float>* Inter);
	//球とAABB
	static bool SphereAndAABB(CollisionSphere* SphereA, CollisionAABB* AABB, Vec3<float>* Inter);
	//球とメッシュ
	static Vec3<float> ClosestPtPoint2Triangle(const Vec3<float>& Pt, const CollisionTriangle& Tri, const Matrix& MeshWorld);
	static bool SphereAndMesh(CollisionSphere* Sphere, CollisionMesh* Mesh, Vec3<float>* Inter);

public:
	static bool CheckPrimitiveHit(CollisionPrimitive* PrimitiveA, CollisionPrimitive* PrimitiveB, Vec3<float>* Inter);
};