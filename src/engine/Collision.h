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
	enum SHAPE { POINT, LINE, SPHERE, PLANE, CAPSULE, AABB, MESH, FLOOR_MESH };

private:
	friend class Collider;
	const SHAPE m_shape;
	Transform* m_world = nullptr;	//ワールドトランスフォーム
	Matrix* m_local = nullptr;

protected:
	//基本的なプリミティブ当たり判定のデバッグ描画
	static std::shared_ptr<GraphicsPipeline>GetPrimitivePipeline();
	//定数バッファ用データ
	struct ConstData
	{
		Matrix m_world = XMMatrixIdentity();
		unsigned int m_hit = 0;
	};

	CollisionPrimitive() = delete;
	CollisionPrimitive(CollisionPrimitive&& arg) = delete;
	CollisionPrimitive(const CollisionPrimitive& arg) = delete;
	CollisionPrimitive(const SHAPE& Shape, Transform* World, Matrix* Local) :m_shape(Shape), m_world(World), m_local(Local) {}
	virtual void DebugDraw(const bool& Hit, Camera& Cam) = 0;	//当たり判定の可視化

public:
	virtual ~CollisionPrimitive() {}
	//ゲッタ
	const SHAPE& GetShape()const { return m_shape; }
	const Matrix& GetWorldMat()
	{
		if (!m_world)return XMMatrixIdentity();
		return m_world->GetMat();
	}
	const Matrix& GetLocalMat()
	{
		if (!m_local)return XMMatrixIdentity();
		return *m_local;
	}
	const float& GetTransformZ()
	{
		return m_world ? m_world->GetPos().z : 0.0f;
	}

	//セッタ
	void AttachWorldTransform(Transform* World) { m_world = World; }
	void AttachLocalMatrix(Matrix* Local) { m_local = Local; }
};

//ポイント
class CollisionPoint : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	std::shared_ptr<ConstantBuffer>m_constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;

public:
	Vec3<float>m_offset;
	CollisionPoint(const Vec3<float>& Pos, Transform* World = nullptr, Matrix* Local = nullptr)
		:CollisionPrimitive(POINT, World, Local) {	m_offset = Pos; }
	Vec3<float>GetWorldPos() 
	{
		return KuroMath::TransformVec3(m_offset, GetLocalMat() * GetWorldMat());
	}
};

//線分
class CollisionLine : public CollisionPrimitive
{
private:
	void DebugDraw(const bool& Hit, Camera& Cam)override;

public:
	//始点座標
	Vec3<float>m_start;
	//方向
	Vec3<float>m_dir;
	//最大距離
	float m_len;

	CollisionLine(const Vec3<float>& Start, const Vec3<float>& Dir, const float& MaxDistance, Transform* World = nullptr, Matrix* Local = nullptr)
		:CollisionPrimitive(LINE, World, Local), m_start(Start), m_dir(Dir), m_len(MaxDistance) {}

	Vec3<float>GetStartWorldPos()
	{
		return KuroMath::TransformVec3(m_start, GetLocalMat() * GetWorldMat());
	}
	Vec3<float>GetEndWorldPos()
	{
		return GetStartWorldPos() + m_dir * m_len;
	}
};

//球
class CollisionSphere : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	std::shared_ptr<ConstantBuffer>m_constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;
	
public:
	Vec3<float>m_offset = { 0,0,0 };
	float m_radius;					//半径
	CollisionSphere(const float& Radius, Transform* World = nullptr, Matrix* Local = nullptr)
		:CollisionPrimitive(SPHERE, World, Local), m_radius(Radius) {}
	Vec3<float>GetCenter()
	{
		return KuroMath::TransformVec3(m_offset, GetLocalMat() * GetWorldMat());
	}
};

//平面
class CollisionPlane : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	void DebugDraw(const bool& Hit, Camera& Cam)override;

public:
	Vec3<float>m_normal = { 0,1,0 };	//板ポリの法線
	float m_distance = 0.0f;				//原点からの距離
	CollisionPlane(const Vec3<float>& Normal = { 0,1,0 }, const float& Distance = 0.0f)
		:CollisionPrimitive(PLANE, nullptr, nullptr), m_normal(Normal), m_distance(Distance) {}
};

//カプセル
class CollisionCapsule : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	std::shared_ptr<ConstantBuffer>m_constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;

public:
	Vec3<float>m_offset;
	Vec3<float>m_sPoint;	//始点
	Vec3<float>m_ePoint;	//終点
	float m_radius;
	CollisionCapsule(const Vec3<float>& StartPt, const Vec3<float>& EndPt, const float& Radius, Transform* World = nullptr, Matrix* Local = nullptr, const Vec3<float>& Offset = Vec3<float>(0, 0, 0))
		:CollisionPrimitive(CAPSULE, World, Local), m_offset(Offset), m_sPoint(StartPt), m_ePoint(EndPt), m_radius(Radius) {}
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
	std::shared_ptr<ConstantBuffer>m_constBuff;
	void DebugDraw(const bool& Hit, Camera& Cam)override;

	//頂点バッファ
	std::shared_ptr<VertexBuffer>m_vertBuff;
	//各軸の最小値と最大値
	Vec3<ValueMinMax>m_pValues;

public:
	CollisionAABB(const Vec3<ValueMinMax>& PValues, Transform* World = nullptr, Matrix* Local = nullptr)
		:CollisionPrimitive(AABB, World, Local) { StructBox(PValues); }

	//ゲッタ
	const Vec3<ValueMinMax>& GetPtValue() { return m_pValues; }
	//セッタ
	void StructBox(const Vec3<ValueMinMax>& PValues);
};

//法線つき三角形（ローカル座標）
class CollisionTriangle
{
private:
	bool m_invalidNormal = true;
	Vec3<float>m_normal;

public:
	Vec3<float>m_p0;
	Vec3<float>m_p1;
	Vec3<float>m_p2;

	//法線セット
	void SetNormal(const Vec3<float>& Normal) 
	{ 
		m_normal = Normal; 
		m_invalidNormal = false;
	}
	//法線の計算
	void CalculateNormal()
	{
		Vec3<float>p0_p1 = m_p1 - m_p0;
		Vec3<float>p0_p2 = m_p2 - m_p0;

		//外積により、２辺に垂直なベクトルを算出
		SetNormal(p0_p1.Cross(p0_p2).GetNormal());
	}

	//法線ゲッタ
	const Vec3<float>& GetNormal()
	{
		if (m_invalidNormal)CalculateNormal();
		assert(!m_normal.IsZero());
		return m_normal;
	}
};

class CollisionMesh : public CollisionPrimitive
{
private:
	friend class Collision;

private:
	std::shared_ptr<VertexBuffer>m_vertBuff;
	std::shared_ptr<ConstantBuffer>m_constBuff;
	std::vector<CollisionTriangle>m_triangles;

	void DebugDraw(const bool& Hit, Camera& Cam)override;
public:
	CollisionMesh(const std::vector<CollisionTriangle>& Triangles, Transform* World = nullptr, Matrix* Local = nullptr, const bool& IsFloor = false)
		: CollisionPrimitive(IsFloor ? FLOOR_MESH : MESH, World, Local)
	{
		SetTriangles(Triangles);
	}

	//ゲッタ
	const std::vector<CollisionTriangle>& GetTriangles()const { return m_triangles; }

	//セッタ
	void SetTriangles(const std::vector<CollisionTriangle>& Triangles);
};

class Collision
{
	//球と球
	static bool SphereAndSphere(CollisionSphere* SphereA, CollisionSphere* SphereB, Vec3<float>* Inter);
	//球と板
	static bool SphereAndPlane(CollisionSphere* Sphere, CollisionPlane* Plane, Vec3<float>* Inter);
	//球と線分
	static bool SphereAndLine(CollisionSphere* Sphere, CollisionLine* Line, Vec3<float>* Inter);
	//球とAABB
	static bool SphereAndAABB(CollisionSphere* SphereA, CollisionAABB* AABB, Vec3<float>* Inter);
	//球とメッシュ
	static Vec3<float> ClosestPtPoint2Triangle(const Vec3<float>& Pt, const CollisionTriangle& Tri, const Matrix& MeshWorld);
	static bool SphereAndMesh(CollisionSphere* Sphere, CollisionMesh* Mesh, Vec3<float>* Inter);
	//点と床メッシュ
	static bool PointAndFloorMesh(CollisionPoint* Point, CollisionMesh* FloorMesh, Vec3<float>* Inter);

public:
	static bool CheckPrimitiveHit(CollisionPrimitive* PrimitiveA, CollisionPrimitive* PrimitiveB, Vec3<float>* Inter);
};