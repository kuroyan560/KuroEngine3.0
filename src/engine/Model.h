#pragma once
#include<string>
#include<vector>
#include"ImportHeader.h"
#include"Skeleton.h"
#include"ModelMesh.h"
#include"Collision.h"

class Model
{
public:
	//ヘッダ（モデル情報）
	ImportHeader m_header;
	//メッシュ
	std::vector<ModelMesh>m_meshes;
	//スケルトン（ボーン構成）
	std::shared_ptr<Skeleton> m_skelton;

	Model(const std::string& Dir, const std::string& FileName) :m_header(Dir, FileName) {}

	//全メッシュをスムージング
	void AllMeshSmoothing()
	{
		for (auto& m : m_meshes)
		{
			m.Smoothing();
		}
	}

	//全メッシュにおける各軸の頂点座標の最小値最大値を取得
	Vec3<ValueMinMax>GetAllMeshPosMinMax();

	void AllMeshBuildTangentAndBiNormal()
	{
		for (auto& m : m_meshes)
		{
			m.BuildTangentAndBiNormal();
		}
	}
};

