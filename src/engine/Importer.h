#pragma once
#include"Singleton.h"
#include<string>
#include<fbxsdk.h>
#include<vector>
#include<memory>
#include<map>
#include<forward_list>
#include"Animation.h"
#include"ModelMesh.h"
#include"Skeleton.h"
class Model;

#include<GLTFSDK/GLTF.h>
#include"StreamReader.h"

class Importer : public Singleton<Importer>
{
	friend class Singleton<Importer>;

	//モデル専用頂点
	using Vertex = ModelMesh::Vertex_Model;

	//エラーメッセージ表示
	void ErrorMessage(const std::string& FuncName, const bool& Fail, const std::string& Comment);
	//HSMロードで用いるデータの読み取り関数（成功すればtrueを返す)
	bool LoadData(FILE* Fp, void* Data, const size_t& Size, const int& ElementNum);
	//HSMセーブで用いるデータの書き込み関数（成功すればtrueを返す)
	bool SaveData(FILE* Fp, const void* Data, const size_t& Size, const int& ElementNum);


#pragma region FBX関連
	struct LoadFbxNode
	{
		std::string name;
		DirectX::XMVECTOR scaling = { 1,1,1,0 };
		DirectX::XMVECTOR rotation = { 0,0,0,0 };
		DirectX::XMVECTOR translation = { 0,0,0,1 };
		Matrix transform;
		LoadFbxNode* parent = nullptr;
		FbxNodeAttribute* attribute = nullptr;
	};
	struct FbxBoneAffect	//ボーンと頂点の関連を記録するためのクラス
	{
		signed short index;
		float weight;
	};
	//ボーンが頂点に与える影響に関する情報テーブル
	//< 頂点インデックス、情報(可変長、４個まで)>
	//５つ以上読み込まれていた場合、頂点読み込み時に無視される
	using BoneTable = std::map<int, std::forward_list<FbxBoneAffect>>;

	FbxManager* fbxManager = nullptr;
	FbxIOSettings* ioSettings = nullptr;
	FbxImporter* fbxImporter = nullptr;

	//FBX関連デバイスの削除処理
	void FbxDeviceDestroy();
	//Fbxノードを再帰的探索
	void ParseNodeRecursive(std::vector<LoadFbxNode>& LoadFbxNodeArray, FbxNode* FbxNode, LoadFbxNode* Parent = nullptr);
	//FbxMesh情報を読み込み
	void LoadFbxMesh(const std::string& Dir, const Skeleton& Skel, ModelMesh& ModelMesh, FbxMesh* FbxMesh);

	//ボーンが頂点に与える影響に関して情報取得
	void LoadBoneAffectTable(const Skeleton& Skel, FbxMesh* FbxMesh, BoneTable& BoneTable);
	//頂点に対応ボーン情報格納
	void SetBoneAffectToVertex(Vertex& Vertex, const int& VertexIdx, BoneTable& BoneTable);
	//頂点情報読み込み
	void LoadFbxVertex(ModelMesh& ModelMesh, FbxMesh* FbxMesh, BoneTable& BoneTable);
	std::string GetFileName(std::string FbxRelativeFileName);
	void LoadFbxMaterial(const std::string& Dir, ModelMesh& ModelMesh, FbxMesh* FbxMesh);
	//アニメーションレイヤーの追跡
	void TraceBoneAnim(const Skeleton& Skel, Skeleton::ModelAnimation& ModelAnimation, FbxNode* FbxNode, FbxAnimLayer* FbxAnimLayer);
	void LoadAnimCurve(FbxAnimCurve* FbxAnimCurve, Animation& Animation);

#pragma endregion

#pragma region glTF関連
	void LoadGLTFPrimitive(ModelMesh& ModelMesh, const Microsoft::glTF::MeshPrimitive& GLTFPrimitive, const Microsoft::glTF::GLTFResourceReader& Reader, const Microsoft::glTF::Document& Doc);
	void PrintDocumentInfo(const Microsoft::glTF::Document& document);
	void PrintResourceInfo(const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader);
	void LoadGLTFMaterial(const MATERIAL_TEX_TYPE& Type, std::weak_ptr<Material>AttachMaterial, const Microsoft::glTF::Image& Img, const std::string& Dir, const Microsoft::glTF::GLTFResourceReader& Reader, const Microsoft::glTF::Document& Doc);
#pragma endregion

	//インポートしたモデル
	std::map<std::string, std::shared_ptr<Model>>models;
	void RegisterImportModel(const std::string& Dir, const std::string& FileName, const std::shared_ptr<Model>& Model)
	{
		models[Dir + FileName] = Model;
	}
	std::shared_ptr<Model> CheckAlreadyExsit(const std::string& Dir, const std::string& FileName);

	//HSMはエンジン専用の多種モデルとの仲介役（他のフォーマット読み込みは時間コストが高い？）
	/*
	std::shared_ptr<Model>LoadHSMModel(const std::string& Dir, const std::string& FileName, const std::string& HSMPath);
	void SaveHSMModel(const std::string& FileNameTail, std::shared_ptr<Model>Model, const FILETIME& LastWriteTime);
	*/

	//HSMが存在する場合をHSMから読み込めるようにするか(HSMのフォーマットが変わったときはフラグをオフにする)
	const bool canLoadHSM = true;

	Importer();

	//※ ファイル名は拡張子つき
	std::shared_ptr<Model> LoadFBXModel(const std::string& Dir, const std::string& FileName, const std::string& Ext);
	std::shared_ptr<Model> LoadGLTFModel(const std::string& Dir, const std::string& FileName, const std::string& Ext);
public:
	~Importer() { FbxDeviceDestroy(); }

	std::shared_ptr<Model>LoadModel(const std::string& Dir, const std::string& FileName);
};

