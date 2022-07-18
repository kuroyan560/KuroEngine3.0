#include "Collision.h"
#include"KuroEngine.h"
#include"Camera.h"
#include<map>

std::shared_ptr<GraphicsPipeline> CollisionPrimitive::GetPrimitivePipeline()
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE;

	if (!PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		PIPELINE_OPTION.wireFrame = true;
		PIPELINE_OPTION.calling = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/CollisionPrimitive/Primitive.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/CollisionPrimitive/Primitive.hlsl", "PSmain", "ps_5_0");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION",DXGI_FORMAT_R32G32B32_FLOAT),
		};

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ワールド行列と衝突判定"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans) };

		PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false) });
	}

	return PIPELINE;
}

void CollisionPoint::DebugDraw(const bool& Hit, Camera& Cam)
{
}

void CollisionSphere::DebugDraw(const bool& Hit,Camera& Cam)
{
	static std::shared_ptr<VertexBuffer>vertBuff;
	static std::shared_ptr<IndexBuffer>idxBuff;

	//頂点バッファとインデックスバッファはクラスで共通のものを使い回す
	if (!vertBuff)
	{
		static const int uMax = 24;
		static const int vMax = 12;
		static const int vertNum = uMax * (vMax + 1);
		static const int idxNum = 2 * vMax * (uMax + 1);

		std::vector<Vec3<float>>vertices(vertNum);
		for (int v = 0; v <= vMax; ++v)
		{
			for (int u = 0; u < uMax; ++u)
			{
				const auto theta = Angle::ConvertToRadian(180.0f * v / vMax);
				const auto phi = Angle::ConvertToRadian(360.0f * u / uMax);
				float fx = static_cast<float>(sin(theta) * cos(phi));
				float fy = static_cast<float>(cos(theta));
				float fz = static_cast<float>(sin(theta) * sin(phi));
				vertices[uMax * v + u] = Vec3<float>(fx, fy, fz);
			}
		}
		vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), vertNum, vertices.data(), "CollisionSphere - VertexBuffer");

		int i = 0;
		std::vector<unsigned int>indices(idxNum);
		for (int v = 0; v < vMax; ++v)
		{
			for (int u = 0; u <= uMax; ++u)
			{
				if (u == uMax)
				{
					indices[i++] = v * uMax;
					indices[i++] = (v + 1) * uMax;
				}
				else
				{
					indices[i++] = (v * uMax) + u;
					indices[i++] = indices[i - 1] + uMax;
				}
			}
		}
		idxBuff = D3D12App::Instance()->GenerateIndexBuffer(idxNum, indices.data(), "CollisionSphere - IndexBuffer");
	}


	if (!m_constBuff)
	{
		m_constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, "Collision_Sphere - ConstantBuffer");
	}

	ConstData constData;
	constData.m_world = XMMatrixScaling(m_radius, m_radius, m_radius) * GetLocalMat() * GetWorldMat() * XMMatrixTranslation(m_offset.x, m_offset.y, m_offset.z);
	constData.m_hit = Hit;
	m_constBuff->Mapping(&constData);

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(CollisionPrimitive::GetPrimitivePipeline());

	KuroEngine::Instance().Graphics().ObjectRender(
		vertBuff,
		idxBuff,
		{ Cam.GetBuff(),m_constBuff }, { CBV,CBV }, GetTransformZ(), true);
}

void CollisionPlane::DebugDraw(const bool& Hit, Camera& Cam)
{
}

void CollisionCapsule::DebugDraw(const bool& Hit, Camera& Cam)
{
}

void CollisionAABB::DebugDraw(const bool& Hit, Camera& Cam)
{
	static std::shared_ptr<IndexBuffer>INDEX_BUFF;
	if (!INDEX_BUFF)
	{
		static const int IDX_NUM = 15;
		std::array<unsigned int, IDX_NUM>indices =
		{
			LU_NZ,RU_NZ,LB_NZ,
			RB_NZ,RB_FZ,RU_NZ,
			RU_FZ,LU_NZ,	LU_FZ,
			LB_NZ,LB_FZ,	RB_FZ,
			LU_FZ,RU_FZ
		};
		INDEX_BUFF = D3D12App::Instance()->GenerateIndexBuffer(IDX_NUM, indices.data(), "CollisionAABB - IndexBuffer");
	}

	//描画に必要なバッファが未生成
	if (!m_constBuff)
	{
		m_constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, "Collision_AABB - ConstantBuffer");
	}

	ConstData constData;
	constData.m_world = GetWorldMat();
	constData.m_hit = Hit;
	m_constBuff->Mapping(&constData);

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(CollisionPrimitive::GetPrimitivePipeline());

	KuroEngine::Instance().Graphics().ObjectRender(
		m_vertBuff,
		INDEX_BUFF,
		{ Cam.GetBuff(),m_constBuff }, { CBV,CBV }, GetTransformZ(), true);
}

void CollisionAABB::StructBox(const Vec3<ValueMinMax>& PValues)
{
	m_pValues = PValues;
	//大小関係がおかしいものがないか確認
	assert(m_pValues.x && m_pValues.y && m_pValues.z);

	if (!m_vertBuff)
	{
		m_vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), VERT_NUM, nullptr, "CollisionAABB - VertexBuffer");
	}

	std::array<Vec3<float>, VERT_NUM>vertices;
	vertices[LU_NZ] = { m_pValues.x.min,m_pValues.y.max,m_pValues.z.min };
	vertices[RU_NZ] = { m_pValues.x.max,m_pValues.y.max,m_pValues.z.min };
	vertices[RB_NZ] = { m_pValues.x.max,m_pValues.y.min,m_pValues.z.min };
	vertices[LB_NZ] = { m_pValues.x.min,m_pValues.y.min,m_pValues.z.min };
	vertices[LU_FZ] = { m_pValues.x.min,m_pValues.y.max,m_pValues.z.max };
	vertices[RU_FZ] = { m_pValues.x.max,m_pValues.y.max,m_pValues.z.max };
	vertices[RB_FZ] = { m_pValues.x.max,m_pValues.y.min,m_pValues.z.max };
	vertices[LB_FZ] = { m_pValues.x.min,m_pValues.y.min,m_pValues.z.max };
	m_vertBuff->Mapping(vertices.data());
}

void CollisionMesh::SetTriangles(const std::vector<CollisionTriangle>& Triangles)
{
	m_triangles = Triangles;

	std::vector<Vec3<float>>vertices;
	for (auto& t : m_triangles)
	{
		vertices.emplace_back(t.m_p0);
		vertices.emplace_back(t.m_p1);
		vertices.emplace_back(t.m_p2);
	}
	m_vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), static_cast<int>(vertices.size()), vertices.data(), "CollisionMesh - VertexBuffer");
	m_constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, "Collision_Mesh - ConstantBuffer");

}

void CollisionMesh::DebugDraw(const bool& Hit, Camera& Cam)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE;
	if (!PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		PIPELINE_OPTION.wireFrame = true;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/CollisionPrimitive/Mesh.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/CollisionPrimitive/Mesh.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ワールド行列と衝突判定"),
		};

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION",DXGI_FORMAT_R32G32B32_FLOAT),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans) };

		PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false) });
	}

	ConstData constData;
	constData.m_world = XMMatrixMultiply(XMMatrixScaling(1.1f, 1.1f, 1.1f), GetWorldMat());
	constData.m_hit = Hit;
	m_constBuff->Mapping(&constData);

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE);

	KuroEngine::Instance().Graphics().ObjectRender(
		m_vertBuff,
		{ Cam.GetBuff(),m_constBuff }, { CBV,CBV }, GetTransformZ(), true);
}


bool Collision::SphereAndSphere(CollisionSphere* SphereA, CollisionSphere* SphereB, Vec3<float>* Inter)
{
	//２つの球のワールド中心座標を求める
	const auto centerA = SphereA->GetCenter();
	const auto centerB = SphereB->GetCenter();

	// 中心点の距離の２乗 <= 半径の和の２乗なら交差
	const float distSq = centerA.DistanceSq(centerB);
	const float radius2 = pow(SphereA->m_radius + SphereB->m_radius, 2.0f);

	if (distSq <= radius2)
	{
		if (Inter)
		{
			//２つの中心間の中心点
			*Inter = centerA.GetCenter(centerB);
		}
		return true;
	}
	return false;
}

bool Collision::SphereAndPlane(CollisionSphere* Sphere, CollisionPlane* Plane, Vec3<float>* Inter)
{
	//球のワールド中心座標を求める
	const auto center = Sphere->GetCenter();

	// 座標系の原点から球の中心座標への距離から
	// 平面の原点距離を減算することで、平面と球の中心との距離が出る
	float dist = XMVector3Dot(center, Plane->m_normal).m128_f32[0] - Plane->m_distance;
	// 距離の絶対値が半径より大きければ当たっていない
	if (fabsf(dist) > Sphere->m_radius)	return false;

	// 擬似交点を計算
	if (Inter)
	{
		// 平面上の再接近点を、疑似交点とする
		*Inter = Plane->m_normal * -dist + center;
	}
	return true;
}

bool Collision::SphereAndAABB(CollisionSphere* SphereA, CollisionAABB* AABB, Vec3<float>* Inter)
{
	//球の中心座標とAABBとの最短距離を求める
	const auto spCenter = KuroMath::TransformVec3(SphereA->m_offset, SphereA->GetWorldMat());

	//AABBの各軸の最小値最大値にワールド変換
	const auto& ptVal = AABB->GetPtValue();

	Vec3<float>ptMin(ptVal.x.min, ptVal.y.min, ptVal.z.min);
	ptMin = KuroMath::TransformVec3(ptMin, AABB->GetWorldMat());
	Vec3<float>ptMax(ptVal.x.max, ptVal.y.max, ptVal.z.max);
	ptMax = KuroMath::TransformVec3(ptMax, AABB->GetWorldMat());

	//回転によって最小・最大が入れ替わっていることがあるので調整
	if (ptMax.x < ptMin.x)std::swap(ptMax.x, ptMin.x);
	if (ptMax.y < ptMin.y)std::swap(ptMax.y, ptMin.y);
	if (ptMax.z < ptMin.z)std::swap(ptMax.z, ptMin.z);

	float distSq = 0.0f;
	if (spCenter.x < ptMin.x)distSq += static_cast<float>(pow((spCenter.x - ptMin.x), 2));
	if (ptMax.x < spCenter.x)distSq += static_cast<float>(pow((spCenter.x - ptMax.x), 2));

	if (spCenter.y < ptMin.y)distSq += static_cast<float>(pow((spCenter.y - ptMin.y), 2));
	if (ptMax.y < spCenter.y)distSq += static_cast<float>(pow((spCenter.y - ptMax.y), 2));

	if (spCenter.z < ptMin.z)distSq += static_cast<float>(pow((spCenter.z - ptMin.z), 2));
	if (ptMax.z < spCenter.z)distSq += static_cast<float>(pow((spCenter.z - ptMax.z), 2));

	if (distSq <= pow(SphereA->m_radius, 2))
	{
		if (Inter)
		{
			//球の中心とAABBの中心間の中心点
			Vec3<float>aabbCenter(ptVal.x.GetCenter(), ptVal.y.GetCenter(), ptVal.z.GetCenter());
			*Inter = spCenter.GetCenter(aabbCenter);
		}
		return true;
	}
	return false;
}

Vec3<float> Collision::ClosestPtPoint2Triangle(const Vec3<float>& Pt, const CollisionTriangle& Tri, const Matrix& MeshWorld)
{
	//三角メッシュの座標をワールド変換
	Vec3<float>p0 = KuroMath::TransformVec3(Tri.m_p0, MeshWorld);
	Vec3<float>p1 = KuroMath::TransformVec3(Tri.m_p1, MeshWorld);
	Vec3<float>p2 = KuroMath::TransformVec3(Tri.m_p2, MeshWorld);

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
	const auto spCenter = KuroMath::TransformVec3(Sphere->m_offset, Sphere->GetWorldMat());

	for (auto& t : Mesh->m_triangles)
	{
		// 球の中心に対する最近接点である三角形上にある点pを見つける
		Vec3<float>closest = ClosestPtPoint2Triangle(spCenter, t, Mesh->GetWorldMat());
		Vec3<float>v = closest - spCenter;
		float distSq = v.Dot(v);

		if (pow(Sphere->m_radius, 2.0f) < distSq)continue;

		if (Inter)*Inter = closest;
		return true;
	}

	return false;
}

bool Collision::PointAndFloorMesh(CollisionPoint* Point, CollisionMesh* FloorMesh, Vec3<float>* Inter)
{
	for (auto& tri : FloorMesh->m_triangles)
	{
		auto normal = tri.GetNormal();

		//法線が下方向なら反転
		if (normal.y < 0)normal.y *= -1;

		//ポイントのワールド座標
		auto pt = Point->GetWorldPos();

		//三角形の頂点A
		auto triPtA = tri.m_p0;

		//床の高さを求める
		float floorY = triPtA.y - 1.0f / normal.y * (normal.x * (pt.x - triPtA.x) + normal.z * (pt.z - triPtA.z));

		//床より上なのでめり込んでいない
		if (floorY < pt.y)continue;

		if (Inter)*Inter = { pt.x,floorY,pt.z };
		return true;
	}
	return false;
}

bool Collision::CheckPrimitiveHit(CollisionPrimitive* PrimitiveA, CollisionPrimitive* PrimitiveB, Vec3<float>* Inter)
{
	//nullチェック
	assert(PrimitiveA && PrimitiveB);

	//点Aと
	if (PrimitiveA->GetShape() == CollisionPrimitive::POINT)
	{
		CollisionPoint* ptA = (CollisionPoint*)PrimitiveA;

		//床メッシュB
		if (PrimitiveB->GetShape() == CollisionPrimitive::FLOOR_MESH)
		{
			return PointAndFloorMesh(ptA, (CollisionMesh*)PrimitiveB, Inter);
		}
	}
	//球Aと
	else if (PrimitiveA->GetShape() == CollisionPrimitive::SPHERE)
	{
		CollisionSphere* sphereA = (CollisionSphere*)PrimitiveA;

		//球B
		if (PrimitiveB->GetShape() == CollisionPrimitive::SPHERE)
		{
			return SphereAndSphere(sphereA, (CollisionSphere*)PrimitiveB, Inter);
		}
		//板B
		else if (PrimitiveB->GetShape() == CollisionPrimitive::PLANE)
		{
			return SphereAndPlane(sphereA, (CollisionPlane*)PrimitiveB, Inter);
		}
		//(AABB)B
		else if (PrimitiveB->GetShape() == CollisionPrimitive::AABB)
		{
			return SphereAndAABB(sphereA, (CollisionAABB*)PrimitiveB, Inter);
		}
		//メッシュB
		else if (PrimitiveB->GetShape() == CollisionPrimitive::MESH)
		{
			return SphereAndMesh(sphereA, (CollisionMesh*)PrimitiveB, Inter);
		}
	}
	//板Aと
	else if (PrimitiveA->GetShape() == CollisionPrimitive::PLANE)
	{
		CollisionPlane* planeA = (CollisionPlane*)PrimitiveA;

		//球B
		if (PrimitiveB->GetShape() == CollisionPrimitive::SPHERE)
		{
			return SphereAndPlane((CollisionSphere*)PrimitiveB, planeA, Inter);
		}
		//板B
		else if (PrimitiveB->GetShape() == CollisionPrimitive::PLANE)
		{
		}
		//(AABB)B
		else if (PrimitiveB->GetShape() == CollisionPrimitive::AABB)
		{
		}
		//メッシュB
		else if (PrimitiveB->GetShape() == CollisionPrimitive::MESH)
		{
		}
	}
	//(AABB)Aと
	else if (PrimitiveA->GetShape() == CollisionPrimitive::AABB)
	{
		CollisionAABB* aabbA = (CollisionAABB*)PrimitiveA;

		//球B
		if (PrimitiveB->GetShape() == CollisionPrimitive::SPHERE)
		{
			return SphereAndAABB((CollisionSphere*)PrimitiveB, aabbA, Inter);
		}
		//(AABB)B
		else if (PrimitiveB->GetShape() == CollisionPrimitive::AABB)
		{
		}
		//メッシュB
		else if (PrimitiveB->GetShape() == CollisionPrimitive::MESH)
		{
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
		//(AABB)B
		else if (PrimitiveB->GetShape() == CollisionPrimitive::AABB)
		{

		}
		//メッシュB
		else if (PrimitiveB->GetShape() == CollisionPrimitive::MESH)
		{
		}
	}
	//床メッシュAと
	else if (PrimitiveA->GetShape() == CollisionPrimitive::FLOOR_MESH)
	{
		CollisionMesh* floorMeshA = (CollisionMesh*)PrimitiveA;

		//点B
		if (PrimitiveB->GetShape() == CollisionPrimitive::POINT)
		{
			return PointAndFloorMesh((CollisionPoint*)PrimitiveB, floorMeshA, Inter);
		}
	}

	//当てはまる組み合わせが用意されていない
	assert(0);
	return false;
}