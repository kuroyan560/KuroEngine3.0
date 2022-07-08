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

void CollisionSphere::DebugDraw(const bool& Hit,Camera& Cam)
{
	static std::shared_ptr<VertexBuffer>VERTEX_BUFF;
	static std::shared_ptr<IndexBuffer>INDEX_BUFF;

	//頂点バッファとインデックスバッファはクラスで共通のものを使い回す
	if (!VERTEX_BUFF)
	{
		static const int U_MAX = 24;
		static const int V_MAX = 12;
		static const int VERTEX_NUM = U_MAX * (V_MAX + 1);
		static const int INDEX_NUM = 2 * V_MAX * (U_MAX + 1);

		std::vector<Vec3<float>>vertices(VERTEX_NUM);
		for (int v = 0; v <= V_MAX; ++v)
		{
			for (int u = 0; u < U_MAX; ++u)
			{
				const auto theta = Angle::ConvertToRadian(180.0f * v / V_MAX);
				const auto phi = Angle::ConvertToRadian(360.0f * u / U_MAX);
				float fx = static_cast<float>(sin(theta) * cos(phi));
				float fy = static_cast<float>(cos(theta));
				float fz = static_cast<float>(sin(theta) * sin(phi));
				vertices[U_MAX * v + u] = Vec3<float>(fx, fy, fz);
			}
		}
		VERTEX_BUFF = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), VERTEX_NUM, vertices.data(), "CollisionSphere - VertexBuffer");

		int i = 0;
		std::vector<unsigned int>indices(INDEX_NUM);
		for (int v = 0; v < V_MAX; ++v)
		{
			for (int u = 0; u <= U_MAX; ++u)
			{
				if (u == U_MAX)
				{
					indices[i++] = v * U_MAX;
					indices[i++] = (v + 1) * U_MAX;
				}
				else
				{
					indices[i++] = (v * U_MAX) + u;
					indices[i++] = indices[i - 1] + U_MAX;
				}
			}
		}
		INDEX_BUFF = D3D12App::Instance()->GenerateIndexBuffer(INDEX_NUM, indices.data(), "CollisionSphere - IndexBuffer");
	}


	if (!constBuff)
	{
		constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, "Collision_Sphere - ConstantBuffer");
	}

	ConstData constData;
	constData.world = XMMatrixScaling(radius, radius, radius) * GetLocalMat() * GetWorldMat() * XMMatrixTranslation(offset.x, offset.y, offset.z);
	constData.hit = Hit;
	constBuff->Mapping(&constData);

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(CollisionPrimitive::GetPrimitivePipeline());

	KuroEngine::Instance().Graphics().ObjectRender(
		VERTEX_BUFF,
		INDEX_BUFF,
		{ Cam.GetBuff(),constBuff }, { CBV,CBV }, GetTransformZ(), true);
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
	if (!constBuff)
	{
		constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, "Collision_AABB - ConstantBuffer");
	}

	ConstData constData;
	constData.world = GetWorldMat();
	constData.hit = Hit;
	constBuff->Mapping(&constData);

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(CollisionPrimitive::GetPrimitivePipeline());

	KuroEngine::Instance().Graphics().ObjectRender(
		vertBuff,
		INDEX_BUFF,
		{ Cam.GetBuff(),constBuff }, { CBV,CBV }, GetTransformZ(), true);
}

void CollisionAABB::StructBox(const Vec3<ValueMinMax>& PValues)
{
	pValues = PValues;
	//大小関係がおかしいものがないか確認
	assert(pValues.x && pValues.y && pValues.z);

	if (!vertBuff)
	{
		vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), VERT_NUM, nullptr, "CollisionAABB - VertexBuffer");
	}

	std::array<Vec3<float>, VERT_NUM>vertices;
	vertices[LU_NZ] = { pValues.x.min,pValues.y.max,pValues.z.min };
	vertices[RU_NZ] = { pValues.x.max,pValues.y.max,pValues.z.min };
	vertices[RB_NZ] = { pValues.x.max,pValues.y.min,pValues.z.min };
	vertices[LB_NZ] = { pValues.x.min,pValues.y.min,pValues.z.min };
	vertices[LU_FZ] = { pValues.x.min,pValues.y.max,pValues.z.max };
	vertices[RU_FZ] = { pValues.x.max,pValues.y.max,pValues.z.max };
	vertices[RB_FZ] = { pValues.x.max,pValues.y.min,pValues.z.max };
	vertices[LB_FZ] = { pValues.x.min,pValues.y.min,pValues.z.max };
	vertBuff->Mapping(vertices.data());
}

void CollisionMesh::SetTriangles(const std::vector<CollisionTriangle>& Triangles)
{
	triangles = Triangles;

	std::vector<Vec3<float>>vertices;
	for (auto& t : triangles)
	{
		vertices.emplace_back(t.p0);
		vertices.emplace_back(t.p1);
		vertices.emplace_back(t.p2);
	}
	vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), static_cast<int>(vertices.size()), vertices.data(), "CollisionMesh - VertexBuffer");
	constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(ConstData), 1, nullptr, "Collision_Mesh - ConstantBuffer");

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
	constData.world = XMMatrixMultiply(XMMatrixScaling(1.1f, 1.1f, 1.1f), GetWorldMat());
	constData.hit = Hit;
	constBuff->Mapping(&constData);

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE);

	KuroEngine::Instance().Graphics().ObjectRender(
		vertBuff,
		{ Cam.GetBuff(),constBuff }, { CBV,CBV }, GetTransformZ(), true);
}


bool Collision::SphereAndSphere(CollisionSphere* SphereA, CollisionSphere* SphereB, Vec3<float>* Inter)
{
	//２つの球のワールド中心座標を求める
	const auto centerA = SphereA->GetCenter();
	const auto centerB = SphereB->GetCenter();

	// 中心点の距離の２乗 <= 半径の和の２乗なら交差
	const float distSq = centerA.DistanceSq(centerB);
	const float radius2 = pow(SphereA->radius + SphereB->radius, 2.0f);

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

bool Collision::SphereAndAABB(CollisionSphere* SphereA, CollisionAABB* AABB, Vec3<float>* Inter)
{
	//球の中心座標とAABBとの最短距離を求める
	const auto spCenter = KuroMath::TransformVec3(SphereA->offset, SphereA->GetWorldMat());

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

	if (distSq <= pow(SphereA->radius, 2))
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

	//Vec3<float>obbCenter(ptVal.x.GetCenter(), ptVal.y.GetCenter(), ptVal.z.GetCenter());
	//obbCenter = KuroMath::TransformVec3(obbCenter, AABB->GetWorldMat());
	//Vec3<float>vec(0, 0, 0);

	//static const Vec3<float>DIR[3] =
	//{
	//	Vec3<float>(-1,0,0),
	//	Vec3<float>(0,1,0),
	//	Vec3<float>(0,0,1)
	//};

	////各軸についてはみ出た部分のベクトルを算術
	//for (int i = 0; i < 3; ++i)
	//{
	//	const auto dir = KuroMath::TransformVec3(DIR[i], AABB->GetWorldMat()).GetNormal();

	//	float len = ptVal[i].max - ptVal[i].min;
	//	if (len <= 0)continue;
	//	float s = (spCenter - obbCenter).Dot(dir);

	//	//sの値からはみ出した部分があればそのベクトルを加算
	//	s = fabs(s);
	//	if (1 < s)
	//	{
	//		vec += dir * (1 - s) * len;
	//	}
	//}

	//float distSq = vec.Length();
	//if (distSq <= pow(SphereA->radius, 2))
	//{
	//	if (Inter)
	//	{
	//		//球の中心とAABBの中心間の中心点
	//		Vec3<float>aabbCenter(ptVal.x.GetCenter(), ptVal.y.GetCenter(), ptVal.z.GetCenter());
	//		*Inter = spCenter.GetCenter(aabbCenter);
	//	}
	//	return true;
	//}
	//return false;
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
	const auto spCenter = KuroMath::TransformVec3(Sphere->offset, Sphere->GetWorldMat());

	for (auto& t : Mesh->triangles)
	{
		// 球の中心に対する最近接点である三角形上にある点pを見つける
		Vec3<float>closest = ClosestPtPoint2Triangle(spCenter, t, Mesh->GetWorldMat());
		Vec3<float>v = closest - spCenter;
		float distSq = v.Dot(v);

		if (pow(Sphere->radius, 2.0f) < distSq)continue;

		if (Inter)*Inter = closest;
		return true;
	}

	return false;
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

	//当てはまる組み合わせが用意されていない
	assert(0);
	return false;
}