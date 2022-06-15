#include "Collision.h"

bool Collision::SphereAndSphere(CollisionSphere* SphereA, CollisionSphere* SphereB, Vec3<float>* Inter)
{
	//２つの球のワールド中心座標を求める
	const auto centerA = KuroMath::TransformVec3(SphereA->localCenter, SphereA->GetWorldMat());
	const auto centerB = KuroMath::TransformVec3(SphereB->localCenter, SphereB->GetWorldMat());

	// 中心点の距離の２乗 <= 半径の和の２乗なら交差
	const float distSq = centerA.DistanceSq(centerB);
	const float radius2 = pow(SphereA->radius + SphereB->radius, 2.0f);

	if (distSq <= radius2)
	{
		if (Inter)
		{
			// Aの半径が0の時座標はBの中心　Bの半径が0の時座標はAの中心　となるよう補完
			float t = SphereB->radius / (SphereA->radius + SphereB->radius);
			*Inter = KuroMath::Lerp(centerA, centerB, t);
		}
		return true;
	}
	return false;
}

Vec3<float> Collision::ClosestPtPoint2Triangle(const Vec3<float>& Pt, const CollisionTriangle& Tri, const Matrix& MeshWorld)
{
	//三角メッシュの座標をワールド変換
	Vec3<float>p0 = KuroMath::TransformVec3(Tri.p0, MeshWorld);
	Vec3<float>p1 = KuroMath::TransformVec3(Tri.p1, MeshWorld);
	Vec3<float>p2 = KuroMath::TransformVec3(Tri.p2, MeshWorld);

	//Ptがp0の外側の頂点領域の中にあるかチェック
	Vec3<float>p0_p1 = p1 - p0;
	Vec3<float>p0_p2 = p2 - p0;
	Vec3<float>p0_pt = Pt - p0;
	float d1 = p0_p1.Dot(p0_pt);
	float d2 = p0_p2.Dot(p0_pt);
	if (d1 <= 0.0f && d2 <= 0.0f)return p0;

	//Ptがp1の外側の頂点領域の中にあるかチェック
	Vec3<float>p1_pt = Pt - p1;
	float d3 = p0_p1.Dot(p1_pt);
	float d4 = p0_p2.Dot(p1_pt);
	if (0.0f <= d3 && d4 <= d3)return p1;

	//Ptがp0_p1の辺領域の中にあるかチェックし、あればp0_p1上に対する射影を返す
	float vc = d1 * d4 - d3 * d2;
	if (vc <= 0.0f && 0.0f <= d1 && d3 <= 0.0f)
	{
		float v = d1 / (d1 - d3);
		return p0 + p0_p1 * v;
	}

	//Ptがp2の外側の頂点領域の中にあるかチェック
	Vec3<float>p2_pt = Pt - p2;
	float d5 = p0_p1.Dot(p2_pt);
	float d6 = p0_p2.Dot(p2_pt);
	if (0.0f <= d6 && d5 <= d6)return p2;

	//Ptがp0_p2の辺領域の中にあるかチェックし、あればPtのp0_p2上に対する射影を返す
	float vb = d5 * d2 - d1 * d6;
	if (vb <= 0.0f && 0.0f <= d2 && d6 <= 0.0f)
	{
		float w = d2 / (d2 - d6);
		return p0 + p0_p2 * w;
	}

	// Ptがp1_p2の辺領域の中にあるかどうかチェックし、あればPtのp1_p2上に対する射影を返す
	float va = d3 * d6 - d5 * d4;
	if (va <= 0.0f && 0.0f <= (d4 - d3) && 0.0f <= (d5 - d6))
	{
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return p1 + (p2 - p1) * w;
	}

	// Ptは面領域の中にある。closestを重心座標を用いて計算する
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	return p0 + p0_p1 * v + p0_p2 * w;
}

bool Collision::SphereAndMesh(CollisionSphere* Sphere, CollisionMesh* Mesh, Vec3<float>* Inter)
{
	const auto spCenter = KuroMath::TransformVec3(Sphere->localCenter, Sphere->GetWorldMat());

	for (auto& t : Mesh->GetTriangles())
	{
		// 球の中心に対する最近接点である三角形上にある点pを見つける
		Vec3<float>closest = ClosestPtPoint2Triangle(spCenter, t, Mesh->GetWorldMat());
		Vec3<float>v = closest - spCenter;
		float distSq = v.Dot(v);

		if (pow(Sphere->radius, 2.0f) < distSq)continue;

		if (Inter)*Inter = closest;
		return true;
	}
}

bool Collision::CheckPrimitiveHit(CollisionPrimitive* PrimitiveA, CollisionPrimitive* PrimitiveB, Vec3<float>* Inter)
{
	//nullチェック
	assert(PrimitiveA && PrimitiveB);

	//球Aと
	if (PrimitiveA->GetShape() == CollisionPrimitive::SPHERE)
	{
		CollisionSphere* sphereA = (CollisionSphere*)PrimitiveA;

		//球B
		if (PrimitiveB->GetShape() == CollisionPrimitive::SPHERE)
		{
			return SphereAndSphere(sphereA, (CollisionSphere*)PrimitiveB, Inter);
		}
		//メッシュB
		else if (PrimitiveB->GetShape() == CollisionPrimitive::MESH)
		{
			return SphereAndMesh(sphereA, (CollisionMesh*)PrimitiveB, Inter);
		}
	}
	//メッシュAと
	else if (PrimitiveA->GetShape() == CollisionPrimitive::MESH)
	{
		CollisionMesh* meshA = (CollisionMesh*)PrimitiveA;

		//球B
		if (PrimitiveB->GetShape() == CollisionPrimitive::SPHERE)
		{
			return SphereAndMesh((CollisionSphere*)PrimitiveB, meshA, Inter);
		}
		//メッシュB
		else if (PrimitiveB->GetShape() == CollisionPrimitive::MESH)
		{
		}
	}

	//当てはまる組み合わせが用意されていない
	assert(0);
	return false;
}
