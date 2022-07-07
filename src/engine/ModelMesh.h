#pragma once
#include"Vec.h"
#include<memory>
#include"Mesh.h"
#include"Material.h"
#include<vector>
#include"Collision.h"

class ModelMesh
{
public:
	//モデル専用頂点クラス
	class Vertex
	{
	public:
		static std::vector<InputLayoutParam>GetInputLayout()
		{
			static std::vector<InputLayoutParam>INPUT_LAYOUT =
			{
				InputLayoutParam("POSITION",DXGI_FORMAT_R32G32B32_FLOAT),
				InputLayoutParam("NORMAL",DXGI_FORMAT_R32G32B32_FLOAT),
				InputLayoutParam("TANGENT",DXGI_FORMAT_R32G32B32_FLOAT),	//接ベクトル
				InputLayoutParam("BINORMAL",DXGI_FORMAT_R32G32B32_FLOAT),	//従ベクトル
				InputLayoutParam("TEXCOORD",DXGI_FORMAT_R32G32_FLOAT),
				InputLayoutParam("BONE_NO",DXGI_FORMAT_R16G16B16A16_SINT),
				InputLayoutParam("WEIGHT",DXGI_FORMAT_R32G32B32A32_FLOAT),
			};
			return INPUT_LAYOUT;
		}

		Vec3<float>pos;
		Vec3<float>normal = { 0,0,0 };
		Vec3<float>tangent = { 0,0,0 };
		Vec3<float>binormal = { 0,0,0 };
		Vec2<float>uv;
		Vec4<signed short>boneIdx = { -1,-1,-1,-1 };
		Vec4<float>boneWeight = { 0,0,0,0 };
	};

	std::shared_ptr<Mesh<ModelMesh::Vertex>>mesh;
	std::shared_ptr<Material> material;

	//スムージング
	void Smoothing();

	//接ベクトルと従ベクトル計算
	void BuildTangentAndBiNormal();

	//当たり判定用三角メッシュ配列取得
	std::vector<CollisionTriangle> GetCollisionTriangles();

	//各軸の頂点座標の最小値と最大値取得
	Vec3<ValueMinMax>GetPosMinMax()const;
};

