#pragma once
#include<string>
#include<vector>
#include"ImportHeader.h"
#include"Skeleton.h"
#include"ModelMesh.h"

struct Model
{
public:
	//ヘッダ（モデル情報）
	ImportHeader header;
	//メッシュ
	std::vector<ModelMesh>meshes;
	//スケルトン（ボーン構成）
	Skeleton skelton;

	Model(const std::string& Dir, const std::string& FileName) :header(Dir, FileName) {}

	void MeshSmoothing()
	{
		for (auto& m : meshes)
		{
			m.Smoothing();
		}
	}
};

