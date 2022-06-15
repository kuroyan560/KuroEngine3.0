#pragma once
#include<vector>
#include<memory>
#include"D3D12App.h"
#include"KuroFunc.h"

//頂点とインデックス情報を持ったメッシュテンプレートクラス（データコンテナ）
template<typename VertexType>
class Mesh
{
	bool invalid = true;
public:
	//メッシュ名
	std::string name;

	//頂点配列（テンプレート)
	std::vector<VertexType>vertices;
	//頂点バッファ
	std::shared_ptr<VertexBuffer>vertBuff;

	//インデックス配列
	std::vector<unsigned int>indices;
	//インデックスバッファ
	std::shared_ptr<IndexBuffer>idxBuff;

	//バッファ生成
	void CreateBuff()
	{
		KuroFunc::ErrorMessage(vertices.empty(), "Mesh", "CreateBuff", "メッシュのバッファ生成をしようとしましたが、頂点が空です\n");

		//頂点バッファ生成
		if(!vertBuff)vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(VertexType), static_cast<int>(vertices.size()), &vertices[0], name.c_str());

		//インデックスバッファ生成
		if (!idxBuff && !indices.empty())idxBuff = D3D12App::Instance()->GenerateIndexBuffer(static_cast<int>(indices.size()), &indices[0], name.c_str());

		invalid = false;
	}
	//マッピング
	void Mapping()
	{
		KuroFunc::ErrorMessage(invalid, "Mesh", "Mapping", "メッシュのバッファのマッピングに失敗、バッファ生成がまだ行われていません\n");
		vertBuff->Mapping(&vertices[0]);
		if (idxBuff)idxBuff->Mapping(&indices[0]);
	}
	//バッファ名セッタ
	void SetNameToBuff(const std::string& Name)
	{
		KuroFunc::ErrorMessage(invalid, "Mesh", "SetNameToBuff", "メッシュのバッファの改名に失敗、バッファ生成がまだ行われていません\n");
		auto wideName = KuroFunc::GetWideStrFromStr(name).c_str();
		vertBuff->SetName(wideName);
		if (idxBuff)idxBuff->SetName(wideName);
	}
};

