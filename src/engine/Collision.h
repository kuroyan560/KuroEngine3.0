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
	enum SHAPE { SPHERE, CAPSULE, AABB, MESH };

private:
	friend class Collider;
	const SHAPE shape;
	Transform* world = nullptr;	//ワールドトランスフォーム
	Matrix* local = nullptr;

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
	CollisionPrimitive(const SHAPE& Shape, Transform* World, Matrix* Local) :shape(Shape), world(World), local(Local) {}
	virtual void DebugDraw(const bool& Hit, Camera& Cam) = 0;	//当たり判定の可視化

public:
	Vec3<float>offset = { 0,0,0 };

	//ゲッタ
	const SHAPE& GetShape()const { return shape; }
	const Matrix& GetWorldMat()
	{
		if (!world)return XMMatrixIdentity();
		return world->GetMat();
	}
	const Matrix& GetLocalMat()
	{
		if (!local)return XMMatrixIdentity();
		return *local;
	}
	const float& GetTransformZ()
	{
		return world ? world->GetPos().z : 0.0f;
	}

	//セッタ
	void AttachWorldTransform(Transform* World) { world = World; }
	void AttachLocalMatrix(Matrix* Local) { local = Local; }
};

//球
class CollisionSphere : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	std::shared_ptr<ConstantBuffer>constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;
	
public:
	float radius;					//半径
	CollisionSphere(const float& Radius, Transform* World = nullptr, Matrix* Local = nullptr)
		:CollisionPrimitive(SPHERE, World, Local), radius(Radius) {}
	Vec3<float>GetCenter()
	{
		return KuroMath::TransformVec3(offset, GetLocalMat() * GetWorldMat());
	}
};

//カプセル
class CollisionCapsule : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	std::shared_ptr<ConstantBuffer>constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;

public:
	Vec3<float>sPoint;	//始点
	Vec3<float>ePoint;	//終点
	float radius;
	Vec3<float>offset;
	CollisionCapsule(const Vec3<float>& StartPt, const Vec3<float>& EndPt, const float& Radius, Transform* World = nullptr, Matrix* Local = nullptr, const Vec3<float>& Offset = Vec3<float>(0, 0, 0))
		:CollisionPrimitive(CAPSULE, World, Local), sPoint(StartPt), ePoint(EndPt), radius(Radius), offset(Offset) {}
};

//AABB(軸並行境界ボックス）、色んな軸で回転すると判定がだめになる
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
	std::shared_ptr<ConstantBuffer>constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>vertBuff;
	//各軸の最小値と最大値
	Vec3<ValueMinMax>pValues;

public:
	CollisionAABB(const Vec3<ValueMinMax>& PValues, Transform* World = nullptr, Matrix* Local = nullptr)
		:CollisionPrimitive(AABB, World, Local) { StructBox(PValues); }

	//ゲッタ
	const Vec3<ValueMinMax>& GetPtValue() { return pValues; }
	//セッタ
	void StructBox(const Vec3<ValueMinMax>& PValues);
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

	void DebugDraw(const bool& Hit, Camera& Cam)override;
public:
	CollisionMesh(const std::vector<CollisionTriangle>& Triangles, Transform* World = nullptr, Matrix* Local = nullptr)
		: CollisionPrimitive(MESH, World, Local)
	{
		SetTriangles(Triangles);
	}

	//ゲッタ
	const std::vector<CollisionTriangle>& GetTriangles()const { return triangles; }

	//セッタ
	void SetTriangles(const std::vector<CollisionTriangle>& Triangles);
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