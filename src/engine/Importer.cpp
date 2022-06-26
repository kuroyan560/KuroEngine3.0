#include "Importer.h"
#include"KuroFunc.h"
#include<Windows.h>
#include"Model.h"
#include"D3D12App.h"

void Importer::ErrorMessage(const std::string& FuncName, const bool& Fail, const std::string& Comment)
{
	KuroFunc::ErrorMessage(Fail, "Importer", FuncName, Comment);
}

bool Importer::LoadData(FILE* Fp, void* Data, const size_t& Size, const int& ElementNum)
{
	return 1 <= fread(Data, Size, ElementNum, Fp);
}

bool Importer::SaveData(FILE* Fp, const void* Data, const size_t& Size, const int& ElementNum)
{
	return 1 <= fwrite(Data, Size, ElementNum, Fp);
}

void Importer::FbxDeviceDestroy()
{
	if (fbxImporter)fbxImporter->Destroy();
	if (ioSettings)ioSettings->Destroy();
	if (fbxManager)fbxManager->Destroy();
}

void Importer::ParseNodeRecursive(std::vector<LoadFbxNode>& LoadFbxNodeArray, FbxNode* FbxNode, LoadFbxNode* Parent)
{
	//ロード済ノード配列に要素追加
	LoadFbxNodeArray.emplace_back();

	LoadFbxNode& node = LoadFbxNodeArray.back();

	//FBX上のノードのポインタ格納
	node.fbxNode = FbxNode;

	//ノード名取得
	node.name = FbxNode->GetName();

	// ローカル変形行列の計算
	FbxDouble3 rotation = FbxNode->LclRotation.Get();
	FbxDouble3 scaling = FbxNode->LclScaling.Get();
	FbxDouble3 translation = FbxNode->LclTranslation.Get();
	node.rotation = { (float)rotation[0], (float)rotation[1], (float)rotation[2], 0.0f };
	node.scaling = { (float)scaling[0], (float)scaling[1], (float)scaling[2], 0.0f };
	node.translation = { (float)translation[0], (float)translation[1], (float)translation[2], 1.0f };

	XMMATRIX matScaling, matRotation, matTranslation;
	matScaling = XMMatrixScalingFromVector(node.scaling);
	matRotation = XMMatrixRotationRollPitchYawFromVector(node.rotation);
	matTranslation = XMMatrixTranslationFromVector(node.translation);

	node.transform = XMMatrixIdentity();
	node.transform *= matScaling;
	node.transform *= matRotation;
	node.transform *= matTranslation;

	//親設定
	if (Parent) 
	{
		node.parent = Parent;
	}

	// FBXノードのメッシュ情報を解析
	node.attribute = FbxNode->GetNodeAttribute();

	// 子ノードに対して再帰呼び出し
	const int childCount = FbxNode->GetChildCount();
	for (int childIdx = 0; childIdx < childCount; childIdx++) {
		auto childName = FbxNode->GetChild(childIdx)->GetName();
		ParseNodeRecursive(LoadFbxNodeArray, FbxNode->GetChild(childIdx), &node);
	}
}

void Importer::LoadBoneAffectTable(const Skeleton& Skel, FbxMesh* FbxMesh, BoneTable& BoneTable)
{
	//スキンの数を取得
	int skinCount = FbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	if (!skinCount)return;

	for (int skinIdx = 0; skinIdx < skinCount; ++skinIdx)
	{
		//i番目のスキンを取得
		FbxSkin* skin = (FbxSkin*)FbxMesh->GetDeformer(skinIdx, FbxDeformer::eSkin);

		//クラスターの数を取得
		int clusterNum = skin->GetClusterCount();

		for (int clusterIdx = 0; clusterIdx < clusterNum; ++clusterIdx)
		{
			//j番目のクラスタを取得
			FbxCluster* cluster = skin->GetCluster(clusterIdx);

			//該当ボーンが影響を与える頂点の数
			int pointNum = cluster->GetControlPointIndicesCount();
			if (!pointNum)continue;

			//クラスター名から影響するボーンのインデックス取得
			std::string clusterName = cluster->GetName();
			int affectBoneIdx = -1;
			for (int boneIdx = 0; boneIdx < Skel.bones.size(); ++boneIdx)
			{
				if (Skel.bones[boneIdx].name != clusterName)continue;

				affectBoneIdx = boneIdx;
				break;
			}

			//ボーンが見つからなかった
			KuroFunc::ErrorMessage(affectBoneIdx == -1, "Imporeter", "LoadBoneAffectTable", "Affect bone index wasn't found.");

			//影響を与える頂点のインデックス配列
			int* pointArray = cluster->GetControlPointIndices();
			//ウェイト配列
			double* weightArray = cluster->GetControlPointWeights();

			//ボーンが影響を与える頂点の情報を取得する
			for (int pointIdx = 0; pointIdx < pointNum; ++pointIdx)
			{
				//頂点インデックスとウェイトを取得
				int vertIdx = pointArray[pointIdx];

				//影響テーブルに格納
				FbxBoneAffect info;
				info.index = affectBoneIdx;
				info.weight = (float)weightArray[pointIdx];
				info.weight = std::clamp(info.weight, 0.0f, 1.0f);

				if (info.weight == 0.0f)continue;	//０なら格納する必要なし

				//頂点インデックス(添字)ごとにリストとして影響を受けるボーンは管理している。
				BoneTable[vertIdx].emplace_front(info);
			}
		}
	}
}

void Importer::LoadFbxMesh(const std::string& Dir, const Skeleton& Skel, ModelMesh& ModelMesh, FbxMesh* FbxMesh)
{
	//ボーンが頂点に与える影響に関する情報テーブル
	BoneTable boneAffectTable;
	//テーブル構築
	LoadBoneAffectTable(Skel, FbxMesh, boneAffectTable);
	//頂点
	LoadFbxVertex(ModelMesh, FbxMesh, boneAffectTable);

	//インデックス
	//ポリゴンの数だけ連番として保存する
	auto polygonNum = FbxMesh->GetPolygonCount();
	for (int polygonIdx = 0; polygonIdx < polygonNum; polygonIdx++)
	{
		//左手系（右周り）
		ModelMesh.mesh->indices.emplace_back(polygonIdx * 3 + 1);
		ModelMesh.mesh->indices.emplace_back(polygonIdx * 3);
		ModelMesh.mesh->indices.emplace_back(polygonIdx * 3 + 2);
	}

	//マテリアル
	LoadFbxMaterial(Dir, ModelMesh, FbxMesh);
}

void Importer::LoadFbxVertex(ModelMesh& ModelMesh, FbxMesh* FbxMesh, BoneTable& BoneTable)
{
	//頂点バッファの取得
	FbxVector4* vertices = FbxMesh->GetControlPoints();
	//インデックスバッファの取得
	int* indices = FbxMesh->GetPolygonVertices();
	//頂点座標の数の取得
	int polygonVertexCount = FbxMesh->GetPolygonVertexCount();

	//UVデータの数
	int uvCount = FbxMesh->GetTextureUVCount();
	//uvsetの名前保存用
	FbxStringList uvsetNames;
	//UVSetの名前リストを取得
	FbxMesh->GetUVSetNames(uvsetNames);
	//UV取得
	FbxArray<FbxVector2>uvBuffers;
	FbxMesh->GetPolygonVertexUVs(uvsetNames.GetStringAt(0), uvBuffers);

	//法線取得
	FbxArray<FbxVector4> normals;
	FbxMesh->GetPolygonVertexNormals(normals);

	//頂点情報の取得
	for (int polygonVertIdx = 0; polygonVertIdx < polygonVertexCount; polygonVertIdx++)
	{
		Vertex vertex;

		//インデックスバッファから頂点番号を取得
		int vertIdx = indices[polygonVertIdx];

		//頂点座標リストから座標を取得
		vertex.pos.x = -vertices[vertIdx][0];
		vertex.pos.y = vertices[vertIdx][1];
		vertex.pos.z = vertices[vertIdx][2];

		//法線リストから法線を取得
		vertex.normal.x = -(float)normals[polygonVertIdx][0];
		vertex.normal.y = (float)normals[polygonVertIdx][1];
		vertex.normal.z = (float)normals[polygonVertIdx][2];

		//UVリストから取得
		vertex.uv.x = (float)uvBuffers[polygonVertIdx][0];
		vertex.uv.y = (float)uvBuffers[polygonVertIdx][1];

		//影響データ表が空じゃない
		if (!BoneTable[vertIdx].empty())
		{
			//適用されるボーンの数
			int count = 0;

			//該当インデックスの影響データ一覧を参照
			for (auto itr = BoneTable[vertIdx].begin(); itr != BoneTable[vertIdx].end(); ++itr)
			{
				//対象の頂点のボーンデータで空な領域にデータを保存
				for (int affectBoneIdx = 0; affectBoneIdx < 4; ++affectBoneIdx)
				{
					//ボーン未登録でないなら
					if (vertex.boneIdx[affectBoneIdx] != -1)continue;	
					vertex.boneIdx[affectBoneIdx] = itr->index;
					vertex.boneWeight[affectBoneIdx] = itr->weight;
					break;
				}
				count++;
			}

			//４つまでしかボーン適用できない
			KuroFunc::ErrorMessage(4 < count, "Importer", "SetBoneAffectToVertex", "頂点読み込みにて５つ以上のボーンが適用されました。１つの頂点につき最大４つのボーンを適用出来ません\n");
		}

		//モデルのメッシュに頂点追加
		ModelMesh.mesh->vertices.emplace_back(vertex);
	}
}

std::string Importer::GetFileName(std::string FbxRelativeFileName)
{
	auto strItr = FbxRelativeFileName.begin();

	for (auto itr = FbxRelativeFileName.begin(); itr != FbxRelativeFileName.end(); itr++)
	{
		if (*itr == '\\')
		{
			*itr = '/';
		}
		if (*itr == '/')
		{
			strItr = itr + 1;
		}
	}
	std::string result;
	for (auto itr = strItr; itr != FbxRelativeFileName.end(); itr++)
	{
		result += *itr;
	}

	return result;
}

void Importer::LoadFbxMaterial(const std::string& Dir, ModelMesh& ModelMesh, FbxMesh* FbxMesh)
{
	FbxNode* meshRootNode = FbxMesh->GetNode();
	//マテリアルの数
	int materialNum = meshRootNode->GetMaterialCount();

	//メッシュにアタッチされる予定のマテリアル名
	std::string attachedMatName;
	if (FbxMesh->GetElementMaterialCount() != 0)
	{
		FbxLayerElementMaterial* element = FbxMesh->GetElementMaterial(0);
		//(FBX上での)マテリアルのインデックス取得
		int materialIndexNum = element->GetIndexArray().GetAt(0);
		//(FBX上での)マテリアル取得
		FbxSurfaceMaterial* surface_material = FbxMesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(materialIndexNum);
		//メッシュに適用するマテリアル名取得
		attachedMatName = surface_material->GetName();
	}

	// マテリアルがない or 名前割当がなければ何もしない
	if (materialNum == 0 || attachedMatName.empty())
	{
		//デフォルトのマテリアル生成
		ModelMesh.material = std::make_shared<Material>();
		return;
	}

	//マテリアルの情報を取得
	for (int i = 0; i < materialNum; ++i)
	{
		FbxSurfaceMaterial* material = meshRootNode->GetMaterial(i);

		if (material == nullptr)continue;
		if (material->GetName() != attachedMatName)continue;	//メッシュが要求するマテリアル名と異なればスルー

		//マテリアル解析
		std::shared_ptr<Material> newMaterial = std::make_shared<Material>();
		newMaterial->name = material->GetName();

		auto& info = newMaterial->constData;

		//Lambert
		FbxSurfaceLambert* lambert = (FbxSurfaceLambert*)material;

		//アンビエント
		info.lambert.ambient.x = (float)lambert->Ambient.Get()[0];
		info.lambert.ambient.y = (float)lambert->Ambient.Get()[1];
		info.lambert.ambient.z = (float)lambert->Ambient.Get()[2];
		info.lambert.ambientFactor = (float)lambert->AmbientFactor.Get();

		//ディフューズ
		info.lambert.diffuse.x = (float)lambert->Diffuse.Get()[0];
		info.lambert.diffuse.y = (float)lambert->Diffuse.Get()[1];
		info.lambert.diffuse.z = (float)lambert->Diffuse.Get()[2];
		info.lambert.diffuseFactor = (float)lambert->DiffuseFactor.Get();

		//放射
		info.lambert.emissive.x = (float)lambert->Emissive.Get()[0];
		info.lambert.emissive.y = (float)lambert->Emissive.Get()[1];
		info.lambert.emissive.z = (float)lambert->Emissive.Get()[2];
		info.lambert.emissiveFactor = (float)lambert->EmissiveFactor.Get();

		//透過度
		info.lambert.transparent = (float)lambert->TransparencyFactor.Get();

		//Phong
		if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			FbxSurfacePhong* phong = (FbxSurfacePhong*)material;

			//スペキュラー
			info.phong.specular.x = (float)phong->Specular.Get()[0];
			info.phong.specular.y = (float)phong->Specular.Get()[1];
			info.phong.specular.z = (float)phong->Specular.Get()[2];
			info.phong.specularFactor = (float)phong->SpecularFactor.Get();

			//光沢
			info.phong.shininess = (float)phong->Shininess.Get();

			//反射
			info.phong.reflection = (float)phong->ReflectionFactor.Get();

			//放射テクスチャ
			FbxFileTexture* emissiveTex = nullptr;
			int emissiveTexNum = phong->Emissive.GetSrcObjectCount();
			if (emissiveTexNum)
			{
				emissiveTex = phong->Emissive.GetSrcObject<FbxFileTexture>(0);
			}

			if (emissiveTex != nullptr)
			{
				auto path = Dir + GetFileName(emissiveTex->GetRelativeFileName());
				//エミッシブマップ
				newMaterial->texBuff[EMISSIVE_TEX] = D3D12App::Instance()->GenerateTextureBuffer(path);
			}
		}

		//PBR
		//ベースカラー
		{
			const auto propBaseColor = FbxSurfaceMaterialUtils::GetProperty("baseColor", material);
			if (propBaseColor.IsValid())
			{
				const FbxFileTexture* baseColorTex = propBaseColor.GetSrcObject<FbxFileTexture>();

				if (baseColorTex)
				{
					auto path = Dir + GetFileName(baseColorTex->GetRelativeFileName());
					newMaterial->texBuff[BASE_COLOR_TEX] = D3D12App::Instance()->GenerateTextureBuffer(path);
				}
				else
				{
					auto baseCol = propBaseColor.Get<FbxDouble3>();
					newMaterial->constData.pbr.baseColor.x = (float)baseCol.Buffer()[0];
					newMaterial->constData.pbr.baseColor.y = (float)baseCol.Buffer()[1];
					newMaterial->constData.pbr.baseColor.z = (float)baseCol.Buffer()[2];
				}
			}
		}
		//金属度
		{
			const auto propMetalness = FbxSurfaceMaterialUtils::GetProperty("metalness", material);
			if (propMetalness.IsValid())
			{
				const FbxFileTexture* metalnessTex = propMetalness.GetSrcObject<FbxFileTexture>();

				if (metalnessTex)
				{
					auto path = Dir + GetFileName(metalnessTex->GetRelativeFileName());
					newMaterial->texBuff[METALNESS_TEX] = D3D12App::Instance()->GenerateTextureBuffer(path);
				}
				else 
				{
					newMaterial->constData.pbr.metalness = propMetalness.Get<float>();
				}
			}
		}
		//PBRスペキュラー
		const auto propSpecular = FbxSurfaceMaterialUtils::GetProperty("specular", material);
		if (propSpecular.IsValid())
		{
			newMaterial->constData.pbr.specular = propSpecular.Get<float>();
		}
		//粗さ
		{
			const auto propRoughness = FbxSurfaceMaterialUtils::GetProperty("specularRoughness", material);
			if (propRoughness.IsValid())
			{
				const FbxFileTexture* roughnessTex = propRoughness.GetSrcObject<FbxFileTexture>();

				if (roughnessTex)
				{
					auto path = Dir + GetFileName(roughnessTex->GetRelativeFileName());
					newMaterial->texBuff[ROUGHNESS_TEX] = D3D12App::Instance()->GenerateTextureBuffer(path);
				}
				else
				{
					newMaterial->constData.pbr.roughness = propRoughness.Get<float>();
				}
			}
		}

		//ディヒューズがテクスチャの情報を持っている
		{
			auto prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
			//FbxFileTextureを取得
			FbxFileTexture* tex = nullptr;
			int textureNum = prop.GetSrcObjectCount<FbxFileTexture>();
			if (0 < textureNum)
			{
				//propからFbxFileTextureを取得
				tex = prop.GetSrcObject<FbxFileTexture>(0);
			}
			else
			{
				//失敗したらマルチテクスチャの可能性を考えてFbxLayeredTextureを指定
				//FbxLayeredTextureからFbxFileTextureを取得
				int layerNum = prop.GetSrcObjectCount<FbxLayeredTexture>();
				if (0 < layerNum)
				{
					tex = prop.GetSrcObject<FbxFileTexture>(0);
				}
			}

			if (tex != nullptr)
			{
				const auto fileName = GetFileName(tex->GetRelativeFileName());
				if (!fileName.empty())
				{
					auto path = Dir + fileName;
					newMaterial->texBuff[COLOR_TEX] = D3D12App::Instance()->GenerateTextureBuffer(path);
				}
			}
		}

		//法線マップ
		const FbxProperty propNormalCamera = FbxSurfaceMaterialUtils::GetProperty("normalCamera", material);
		if (propNormalCamera.IsValid())
		{
			const FbxFileTexture* normalTex = propNormalCamera.GetSrcObject<FbxFileTexture>();
			if (normalTex)
			{
				auto path = Dir + GetFileName(normalTex->GetRelativeFileName());
				newMaterial->texBuff[NORMAL_TEX] = D3D12App::Instance()->GenerateTextureBuffer(path);
			}
		}

		ModelMesh.material = newMaterial;
		break;
	}
}

void Importer::LoadBoneAnim(const LoadFbxNode& BoneNode, Skeleton::ModelAnimation& ModelAnimation, FbxAnimLayer* FbxAnimLayer)
{
	//FBXでのアニメーション読み込み防止
	assert(0);

	//ボーン単位アニメーション取得
	auto& boneAnim = ModelAnimation.boneAnim[BoneNode.name];
	auto fbxNode = BoneNode.fbxNode;

	FbxAnimCurve* animCurve;

	////座標
	//animCurve = fbxNode->LclTranslation.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
	//if (animCurve)LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::POS_X]);

	//animCurve = fbxNode->LclTranslation.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	//if (animCurve)	LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::POS_Y]);

	//animCurve = fbxNode->LclTranslation.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	//if (animCurve)	LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::POS_Z]);

	////DirectXの座標軸に合わせる
	//for (auto& key : boneAnim.anims[Skeleton::BoneAnimation::POS_X].keyFrames)
	//{
	//	key.value = -key.value;
	//}

	////回転
	//animCurve = fbxNode->LclRotation.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
	//if (animCurve)LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::ROTATE_X]);

	//animCurve = fbxNode->LclRotation.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	//if (animCurve)	LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::ROTATE_Y]);

	//animCurve = fbxNode->LclRotation.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	//if (animCurve)	LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::ROTATE_Z]);

	////回転のみ弧度法なのでラジアンに変換
	//for (int i = 0; i < 3; ++i)
	//{
	//	for (auto& key : boneAnim.anims[Skeleton::BoneAnimation::ROTATE_X + i].keyFrames)
	//	{
	//		key.value = Angle::ConvertToRadian(key.value);
	//	}
	//}

	////スケール
	//animCurve = fbxNode->LclScaling.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
	//if (animCurve)LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::SCALE_X]);

	//animCurve = fbxNode->LclScaling.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	//if (animCurve)	LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::SCALE_Y]);

	//animCurve = fbxNode->LclScaling.GetCurve(FbxAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	//if (animCurve)	LoadAnimCurve(animCurve, boneAnim.anims[Skeleton::BoneAnimation::SCALE_Z]);
}

/*
void Importer::LoadAnimCurve(FbxAnimCurve* FbxAnimCurve, Animation& Animation)
{
	FbxTimeSpan interval;

	static const auto ONE_FRAME_VALUE = FbxTime::GetOneFrameValue(FbxTime::eFrames60);


	if (FbxAnimCurve->GetTimeInterval(interval)) {
		FbxLongLong start = interval.GetStart().Get();
		FbxLongLong end = interval.GetStop().Get();
		Animation.startFrame = start / ONE_FRAME_VALUE;
		Animation.endFrame =  end / ONE_FRAME_VALUE;
	}

	int lKeyCount = FbxAnimCurve->KeyGetCount();

	int lCount;

	for (lCount = 0; lCount < lKeyCount; lCount++)
	{
		float lKeyValue = static_cast<float>(FbxAnimCurve->KeyGetValue(lCount));
		FbxTime lKeyTime = FbxAnimCurve->KeyGetTime(lCount);

		KeyFrame keyFrame{};

		keyFrame.frame = lKeyTime.Get() / ONE_FRAME_VALUE;
		keyFrame.value = lKeyValue;

		Animation.keyFrames.emplace_back(keyFrame);
	}
}
*/

void Importer::LoadGLTFPrimitive(ModelMesh& ModelMesh, const Microsoft::glTF::MeshPrimitive& GLTFPrimitive, const Microsoft::glTF::Skin* GLTFSkin, const Microsoft::glTF::GLTFResourceReader& Reader, const Microsoft::glTF::Document& Doc)
{
	using namespace Microsoft::glTF;

	// 頂点位置情報アクセッサの取得
	auto& idPos = GLTFPrimitive.GetAttributeAccessorId(ACCESSOR_POSITION);
	auto& accPos = Doc.accessors.Get(idPos);
	auto vertPos = Reader.ReadBinaryData<float>(Doc, accPos);
	// 法線情報アクセッサの取得
	auto& idNrm = GLTFPrimitive.GetAttributeAccessorId(ACCESSOR_NORMAL);
	auto& accNrm = Doc.accessors.Get(idNrm);
	auto vertNrm = Reader.ReadBinaryData<float>(Doc, accNrm);
	// テクスチャ座標情報アクセッサの取得
	auto& idUV = GLTFPrimitive.GetAttributeAccessorId(ACCESSOR_TEXCOORD_0);
	auto& accUV = Doc.accessors.Get(idUV);
	auto vertUV = Reader.ReadBinaryData<float>(Doc, accUV);

	std::vector<uint8_t>vertJoint;
	if (GLTFPrimitive.HasAttribute(ACCESSOR_JOINTS_0))
	{
		ErrorMessage("LoadGLTFPrimitive", !GLTFSkin, "This primitive has joint's info,but any skin node didn't be passed.");
		auto& idJoint = GLTFPrimitive.GetAttributeAccessorId(ACCESSOR_JOINTS_0);
		auto& accJoint = Doc.accessors.Get(idJoint);
		vertJoint = Reader.ReadBinaryData<uint8_t>(Doc, accJoint);
	}

	std::vector<float>vertWeight;
	if (GLTFPrimitive.HasAttribute(ACCESSOR_WEIGHTS_0))
	{
		auto& idWeight = GLTFPrimitive.GetAttributeAccessorId(ACCESSOR_WEIGHTS_0);
		auto& accWeight = Doc.accessors.Get(idWeight);
		vertWeight = Reader.ReadBinaryData<float>(Doc, accWeight);
	}

	auto vertexCount = accPos.count;
	for (int vertIdx = 0; vertIdx < vertexCount; ++vertIdx)
	{
		// 頂点データの構築
		int vid0 = 3 * vertIdx, vid1 = 3 * vertIdx + 1, vid2 = 3 * vertIdx + 2;
		int tid0 = 2 * vertIdx, tid1 = 2 * vertIdx + 1;
		int jid0 = 4 * vertIdx, jid1 = 4 * vertIdx + 1, jid2 = 4 * vertIdx + 2, jid3 = 4 * vertIdx + 3;

		ModelMesh::Vertex_Model vertex;
		vertex.pos = { vertPos[vid0],vertPos[vid1],vertPos[vid2] };
		vertex.normal = { vertNrm[vid0],vertNrm[vid1],vertNrm[vid2] };
		vertex.uv = { vertUV[tid0],vertUV[tid1] };

		if (!vertWeight.empty())
		{
			vertex.boneWeight = { vertWeight[jid0],vertWeight[jid1],vertWeight[jid2],vertWeight[jid3] };
		}
		if (!vertJoint.empty())
		{
			//ボーン割当がない場合 -1 のままにする必要がある
			auto joint0 = atoi(GLTFSkin->jointIds[vertJoint[jid0]].c_str());
			auto joint1 = atoi(GLTFSkin->jointIds[vertJoint[jid1]].c_str());
			auto joint2 = atoi(GLTFSkin->jointIds[vertJoint[jid2]].c_str());
			auto joint3 = atoi(GLTFSkin->jointIds[vertJoint[jid3]].c_str());
			
			if (vertex.boneWeight.x)vertex.boneIdx.x = static_cast<signed short>(joint0);
			if (vertex.boneWeight.y)vertex.boneIdx.y = static_cast<signed short>(joint1);
			if (vertex.boneWeight.z)vertex.boneIdx.z = static_cast<signed short>(joint2);
			if (vertex.boneWeight.w)vertex.boneIdx.w = static_cast<signed short>(joint3);
		}

		ModelMesh.mesh->vertices.emplace_back(vertex);
	}

	//タンジェント情報アクセッサ取得
	if (GLTFPrimitive.HasAttribute(ACCESSOR_TANGENT))
	{
		auto& idTangent = GLTFPrimitive.GetAttributeAccessorId(ACCESSOR_TANGENT);
		auto& accTangent = Doc.accessors.Get(idTangent);
		auto vertTangent = Reader.ReadBinaryData<float>(Doc, accTangent);
		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			int vid0 = 3 * i, vid1 = 3 * i + 1, vid2 = 3 * i + 2;
			ModelMesh.mesh->vertices[i].tangent = { vertTangent[vid0],vertTangent[vid1],vertTangent[vid2] };
			ModelMesh.mesh->vertices[i].binormal = ModelMesh.mesh->vertices[i].normal.Cross(ModelMesh.mesh->vertices[i].tangent);
		}
	}

	// 頂点インデックス用アクセッサの取得
	auto& idIndex = GLTFPrimitive.indicesAccessorId;
	auto& accIndex = Doc.accessors.Get(idIndex);

	// インデックスデータ
	std::vector<uint16_t>indices = Reader.ReadBinaryData<uint16_t>(Doc, accIndex);
	auto idxCount = accIndex.count;
	for (int i = 0; i < idxCount; i += 3)
	{
		ModelMesh.mesh->indices.emplace_back(static_cast<unsigned int>(indices[i]));
		ModelMesh.mesh->indices.emplace_back(static_cast<unsigned int>(indices[i + 1]));
		ModelMesh.mesh->indices.emplace_back(static_cast<unsigned int>(indices[i + 2]));
	}
}

void Importer::PrintDocumentInfo(const Microsoft::glTF::Document& document)
{
	// Asset Info
	std::cout << "Asset Version:    " << document.asset.version << "\n";
	std::cout << "Asset MinVersion: " << document.asset.minVersion << "\n";
	std::cout << "Asset Generator:  " << document.asset.generator << "\n";
	std::cout << "Asset Copyright:  " << document.asset.copyright << "\n\n";

	// Scene Info
	std::cout << "Scene Count: " << document.scenes.Size() << "\n";

	if (document.scenes.Size() > 0U)
	{
		std::cout << "Default Scene Index: " << document.GetDefaultScene().id << "\n\n";
	}
	else
	{
		std::cout << "\n";
	}

	// Entity Info
	std::cout << "Node Count:     " << document.nodes.Size() << "\n";
	std::cout << "Camera Count:   " << document.cameras.Size() << "\n";
	std::cout << "Material Count: " << document.materials.Size() << "\n\n";

	// Mesh Info
	std::cout << "Mesh Count: " << document.meshes.Size() << "\n";
	std::cout << "Skin Count: " << document.skins.Size() << "\n\n";

	// Texture Info
	std::cout << "Image Count:   " << document.images.Size() << "\n";
	std::cout << "Texture Count: " << document.textures.Size() << "\n";
	std::cout << "Sampler Count: " << document.samplers.Size() << "\n\n";

	// Buffer Info
	std::cout << "Buffer Count:     " << document.buffers.Size() << "\n";
	std::cout << "BufferView Count: " << document.bufferViews.Size() << "\n";
	std::cout << "Accessor Count:   " << document.accessors.Size() << "\n\n";

	// Animation Info
	std::cout << "Animation Count: " << document.animations.Size() << "\n\n";

	for (const auto& extension : document.extensionsUsed)
	{
		std::cout << "Extension Used: " << extension << "\n";
	}

	if (!document.extensionsUsed.empty())
	{
		std::cout << "\n";
	}

	for (const auto& extension : document.extensionsRequired)
	{
		std::cout << "Extension Required: " << extension << "\n";
	}

	if (!document.extensionsRequired.empty())
	{
		std::cout << "\n";
	}
}

void Importer::PrintResourceInfo(const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader)
{
	using namespace Microsoft::glTF;
	// Use the resource reader to get each mesh primitive's position data
	for (const auto& mesh : document.meshes.Elements())
	{
		std::cout << "Mesh: " << mesh.id << "\n";

		for (const auto& meshPrimitive : mesh.primitives)
		{
			std::string accessorId;

			if (meshPrimitive.TryGetAttributeAccessorId(ACCESSOR_POSITION, accessorId))
			{
				const Accessor& accessor = document.accessors.Get(accessorId);

				const auto data = resourceReader.ReadBinaryData<float>(document, accessor);
				const auto dataByteLength = data.size() * sizeof(float);

				std::cout << "MeshPrimitive: " << dataByteLength << " bytes of position data\n";
			}
		}

		std::cout << "\n";
	}

	// Use the resource reader to get each image's data
	for (const auto& image : document.images.Elements())
	{
		std::string filename;

		if (image.uri.empty())
		{
			assert(!image.bufferViewId.empty());

			auto& bufferView = document.bufferViews.Get(image.bufferViewId);
			auto& buffer = document.buffers.Get(bufferView.bufferId);

			filename += buffer.uri; //NOTE: buffer uri is empty if image is stored in GLB binary chunk
		}
		else if (IsUriBase64(image.uri))
		{
			filename = "Data URI";
		}
		else
		{
			filename = image.uri;
		}

		auto data = resourceReader.ReadBinaryData(document, image);

		std::cout << "Image: " << image.id << "\n";
		std::cout << "Image: " << data.size() << " bytes of image data\n";

		if (!filename.empty())
		{
			std::cout << "Image filename: " << filename << "\n\n";
		}
	}
}

std::shared_ptr<Model> Importer::CheckAlreadyExsit(const std::string& Dir, const std::string& FileName)
{
	//生成済
	for (auto& m : models)
	{
		//既に生成しているものを渡す
		if (m.first == Dir + FileName)return m.second;
	}

	return std::shared_ptr<Model>();
}

/*
std::shared_ptr<Model> Importer::LoadHSMModel(const std::string& Dir, const std::string& FileName, const std::string& Path)
{
	static const std::string FUNC_NAME = "LoadHSMModel";
	static const std::string MSG_TAIL = "の読み取りに失敗\n";
	std::shared_ptr<Model>model = std::make_shared<Model>(Dir, FileName);

	FILE* fp;
	fopen_s(&fp, Path.c_str(), "rb");
	ErrorMessage(FUNC_NAME, fp == nullptr, ".hsmファイルのバイナリ読み取りモードでのオープンに失敗\n");

	//最終更新日時（データ格納はしない）
	FILETIME dummy;
	ErrorMessage(FUNC_NAME, !LoadData(fp, &dummy, sizeof(dummy), 1), "最終更新日時(ダミー)" + MSG_TAIL);

	size_t size;

	//メッシュの数
	unsigned short meshNum;
	ErrorMessage(FUNC_NAME, !LoadData(fp, &meshNum, sizeof(meshNum), 1), "メッシュの数" + MSG_TAIL);

	//メッシュ生成
	model->meshes.resize(meshNum);

	//メッシュ情報
	for (int meshIdx = 0; meshIdx < meshNum; ++meshIdx)
	{
		//メッシュ取得
		auto& mesh = model->meshes[meshIdx];
		mesh.mesh = std::make_shared<Mesh<ModelMesh::Vertex_Model>>();
		mesh.material = std::make_shared<Material>();

		//メッシュ名サイズ
		ErrorMessage(FUNC_NAME, !LoadData(fp, &size, sizeof(size), 1), "メッシュ名サイズ" + MSG_TAIL);
		//メッシュ名
		mesh.mesh->name.resize(size);
		ErrorMessage(FUNC_NAME, !LoadData(fp, &mesh.mesh->name[0], size, 1), "メッシュ名" + MSG_TAIL);

		//頂点数
		unsigned short vertNum;
		ErrorMessage(FUNC_NAME, !LoadData(fp, &vertNum, sizeof(vertNum), 1), "頂点数" + MSG_TAIL);
		//頂点情報
		mesh.mesh->vertices.resize(vertNum);
		ErrorMessage(FUNC_NAME, !LoadData(fp, &mesh.mesh->vertices[0], sizeof(ModelMesh::Vertex_Model), vertNum), "頂点情報" + MSG_TAIL);

		//インデックス数
		unsigned short idxNum;
		ErrorMessage(FUNC_NAME, !LoadData(fp, &idxNum, sizeof(idxNum), 1), "インデックス数" + MSG_TAIL);
		//インデックス情報
		mesh.mesh->indices.resize(idxNum);
		ErrorMessage(FUNC_NAME, !LoadData(fp, &mesh.mesh->indices[0], sizeof(unsigned int), idxNum), "インデックス情報" + MSG_TAIL);

		//マテリアル取得
		auto& material = mesh.material;
		//マテリアル名サイズ
		ErrorMessage(FUNC_NAME, !LoadData(fp, &size, sizeof(size), 1), "マテリアル名サイズ" + MSG_TAIL);
		//マテリアル名
		material->name.resize(size);
		ErrorMessage(FUNC_NAME, !LoadData(fp, &material->name[0], size, 1), "マテリアル名" + MSG_TAIL);
		//マテリアル情報
		ErrorMessage(FUNC_NAME, !LoadData(fp, &material->constData, sizeof(Material::ConstData), 1), "マテリアル情報" + MSG_TAIL);

		//画像ファイルテクスチャ保持フラグ
		unsigned char haveTex[MATERIAL_TEX_TYPE_NUM];
		ErrorMessage(FUNC_NAME, !LoadData(fp, haveTex, sizeof(unsigned char), MATERIAL_TEX_TYPE_NUM), "画像ファイルテクスチャ保持フラグ" + MSG_TAIL);
		//画像ファイルテクスチャパス情報
		for (int texIdx = 0; texIdx < MATERIAL_TEX_TYPE_NUM; ++texIdx)
		{
			if (!haveTex[texIdx])continue;

			//ファイルパスサイズ
			ErrorMessage(FUNC_NAME, !LoadData(fp, &size, sizeof(size), 1), "ファイルパスサイズ" + MSG_TAIL);
			//ファイルパス
			material->textures[texIdx].path.resize(size);
			ErrorMessage(FUNC_NAME, !LoadData(fp, &material->textures[texIdx].path[0], size, 1), "画像ファイルパス" + MSG_TAIL);
		}

		mesh.material->CreateBuff();
		mesh.mesh->CreateBuff();
	}

	//スケルトン取得
	auto& skel = model->skelton;
	//ボーンの数
	unsigned short boneNum;
	ErrorMessage(FUNC_NAME, !LoadData(fp, &boneNum, sizeof(boneNum), 1), "ボーン数" + MSG_TAIL);
	skel.bones.resize(boneNum);

	//ボーン情報
	for (int boneIdx = 0; boneIdx < boneNum; ++boneIdx)
	{
		//ボーン取得
		auto& bone = skel.bones[boneIdx];
		//ボーン名サイズ
		ErrorMessage(FUNC_NAME, !LoadData(fp, &size, sizeof(size), 1), "ボーン名サイズ" + MSG_TAIL);
		//ボーン名
		bone.name.resize(size);
		ErrorMessage(FUNC_NAME, !LoadData(fp, &bone.name[0], size, 1), "ボーン名" + MSG_TAIL);

		//名前以外のボーン情報
		ErrorMessage(FUNC_NAME, !LoadData(fp, &bone.parent, Bone::GetSizeWithOutName(), 1), "ボーン情報" + MSG_TAIL);
	}
	skel.CreateBoneTree();

	fclose(fp);

	RegisterImportModel(Dir, FileName, model);

	return model;
}

void Importer::SaveHSMModel(const std::string& FileNameTail, std::shared_ptr<Model> Model, const FILETIME& LastWriteTime)
{
	static const std::string FUNC_NAME = "SaveHSMModel";
	static const std::string MSG_TAIL = "の書き込みに失敗\n";

	FILE* fp;
	fopen_s(&fp, (Model->header.dir + Model->header.GetModelName() + FileNameTail + ".hsm").c_str(), "wb");
	ErrorMessage(FUNC_NAME, fp == nullptr, ".hsmファイルのバイナリ書き込みモードでのオープンに失敗\n");

	//最終更新日時
	ErrorMessage(FUNC_NAME, !SaveData(fp, &LastWriteTime, sizeof(LastWriteTime), 1), "最終更新日時" + MSG_TAIL);

	size_t size;

	//メッシュの数
	unsigned short meshNum = Model->meshes.size();
	ErrorMessage(FUNC_NAME, !SaveData(fp, &meshNum, sizeof(meshNum), 1), "メッシュの数" + MSG_TAIL);

	//メッシュ情報
	for (int meshIdx = 0; meshIdx < meshNum; ++meshIdx)
	{
		//メッシュ取得
		auto& mesh = Model->meshes[meshIdx];
		//メッシュ名サイズ
		size = mesh.mesh->name.length();
		ErrorMessage(FUNC_NAME, !SaveData(fp, &size, sizeof(size), 1), "メッシュ名サイズ" + MSG_TAIL);
		//メッシュ名
		ErrorMessage(FUNC_NAME, !SaveData(fp, mesh.mesh->name.data(), size, 1), "メッシュ名" + MSG_TAIL);

		//頂点数
		unsigned short vertNum = mesh.mesh->vertices.size();
		ErrorMessage(FUNC_NAME, !SaveData(fp, &vertNum, sizeof(vertNum), 1), "頂点数" + MSG_TAIL);
		//頂点情報
		ErrorMessage(FUNC_NAME, !SaveData(fp, mesh.mesh->vertices.data(), sizeof(ModelMesh::Vertex_Model), vertNum), "頂点情報" + MSG_TAIL);

		//インデックス数
		unsigned short idxNum = mesh.mesh->indices.size();
		ErrorMessage(FUNC_NAME, !SaveData(fp, &idxNum, sizeof(idxNum), 1), "インデックス数" + MSG_TAIL);
		//インデックス情報
		ErrorMessage(FUNC_NAME, !SaveData(fp, mesh.mesh->indices.data(), sizeof(unsigned int), idxNum), "インデックス情報" + MSG_TAIL);

		//マテリアル取得
		auto& material = mesh.material;
		//マテリアル名サイズ
		size = material->name.length();
		ErrorMessage(FUNC_NAME, !SaveData(fp, &size, sizeof(size), 1), "マテリアル名サイズ" + MSG_TAIL);
		//マテリアル名
		ErrorMessage(FUNC_NAME, !SaveData(fp, material->name.data(), size, 1), "マテリアル名" + MSG_TAIL);
		//マテリアル情報
		ErrorMessage(FUNC_NAME, !SaveData(fp, &material->constData, sizeof(Material::ConstData), 1), "マテリアル情報" + MSG_TAIL);

		//画像ファイルテクスチャ保持フラグ
		unsigned char haveTex[MATERIAL_TEX_TYPE_NUM] = { 0 };
		for (int texIdx = 0; texIdx < MATERIAL_TEX_TYPE_NUM; ++texIdx)
		{
			if (material->textures[texIdx].path.empty())continue;

			//テクスチャが割り当てられている
			haveTex[texIdx] = 1;
		}
		ErrorMessage(FUNC_NAME, !SaveData(fp, haveTex, sizeof(unsigned char), MATERIAL_TEX_TYPE_NUM), "マテリアルのテクスチャ保持フラグ" + MSG_TAIL);

		//画像ファイルテクスチャパス情報
		for(int texIdx = 0;texIdx < MATERIAL_TEX_TYPE_NUM;++texIdx)
		{
			if (!haveTex[texIdx])continue;

			//画像ファイルパスサイズ
			size = material->textures[texIdx].path.length();
			ErrorMessage(FUNC_NAME, !SaveData(fp, &size, sizeof(size), 1), "テクスチャ画像ファイルパスサイズ" + MSG_TAIL);
			ErrorMessage(FUNC_NAME, !SaveData(fp, material->textures[texIdx].path.data(), size, 1), "テクスチャ画像ファイルパス" + MSG_TAIL);
		}
	}

	//スケルトン取得
	auto& skel = Model->skelton;
	//ボーンの数
	unsigned short boneNum = skel.bones.size();
	ErrorMessage(FUNC_NAME, !SaveData(fp, &boneNum, sizeof(boneNum), 1), "ボーン数" + MSG_TAIL);
	//ボーン情報
	for (int boneIdx = 0; boneIdx < boneNum; ++boneIdx)
	{
		//ボーン取得
		auto& bone = skel.bones[boneIdx];
		//ボーン名サイズ
		size = bone.name.length();
		ErrorMessage(FUNC_NAME, !SaveData(fp, &size, sizeof(size), 1), "ボーン名サイズ" + MSG_TAIL);
		//ボーン名
		ErrorMessage(FUNC_NAME, !SaveData(fp, bone.name.data(), size, 1), "ボーン名" + MSG_TAIL);
		//ボーン情報
		ErrorMessage(FUNC_NAME, !SaveData(fp, &bone.parent, Bone::GetSizeWithOutName(), 1), "ボーン情報" + MSG_TAIL);
	}

	fclose(fp);
}
*/

Importer::Importer()
{
	static const std::string FUNC_NAME = "コンストラクタ";

	//マネージャ生成
	fbxManager = FbxManager::Create();
	ErrorMessage(FUNC_NAME, fbxManager == nullptr, "FBXマネージャ生成に失敗\n");

	//IOSettingを生成
	ioSettings = FbxIOSettings::Create(fbxManager, IOSROOT);
	ErrorMessage(FUNC_NAME, ioSettings == nullptr, "FBXIOSetting生成に失敗\n");

	//インポータ生成
	fbxImporter = FbxImporter::Create(fbxManager, "");
	ErrorMessage(FUNC_NAME, fbxImporter == nullptr, "FBXインポータ生成に失敗\n");
}

std::shared_ptr<Model> Importer::LoadFBXModel(const std::string& Dir, const std::string& FileName, const std::string& Ext)
{
	printf("glTFロード\nDir : %s , FileName : %s\n", Dir.c_str(), FileName.c_str());

	static const std::string FUNC_NAME = "LoadFBX";
	static const std::string HSM_TAIL = "_fbx";

	std::shared_ptr<Model>result;

	//既に生成しているか
	result = CheckAlreadyExsit(Dir, FileName);
	if (result)return result;	//生成していたらそれを返す

	//モデル名取得(ファイル名から拡張子を除いたもの)
	auto modelName = FileName;
	modelName.erase(modelName.size() - Ext.size());

	//ファイルパス
	const auto path = Dir + FileName;

	/*
	//fbxファイルの最終更新日時を読み取る
	FILETIME fbxLastWriteTime;
	HANDLE fbxFile = CreateFile(
		KuroFunc::GetWideStrFromStr(path).c_str(),
		0,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	ErrorMessage(FUNC_NAME, !GetFileTime(fbxFile, NULL, NULL, &fbxLastWriteTime), "fbxファイルの最終更新日時読み取りに失敗\n");

	//hsmファイルが存在するか
	const auto hsmPath = Dir + modelName + HSM_TAIL + ".hsm";
	if (canLoadHSM && KuroFunc::ExistFile(hsmPath))
	{
		//hsmファイルに記録されたモデルの最終更新日時データを読み取る
		FILETIME modelLastWriteTime;
		FILE* fp;
		fopen_s(&fp, hsmPath.c_str(), "rb");
		if (fp != NULL)
		{
			ErrorMessage(FUNC_NAME, fread(&modelLastWriteTime, sizeof(FILETIME), 1, fp) < 1, "hsmファイルの最終更新日時読み取りに失敗\n");
			fclose(fp);
		}

		//fbxファイルの最終更新日時と比較、同じならhsmファイルをロードしてそれを返す
		if (modelLastWriteTime.dwHighDateTime == fbxLastWriteTime.dwHighDateTime
			&& modelLastWriteTime.dwLowDateTime == fbxLastWriteTime.dwLowDateTime)
		{
			return LoadHSMModel(Dir, FileName, hsmPath);
		}
	}
	*/

	//ファイルの存在を確認
	ErrorMessage(FUNC_NAME, !KuroFunc::ExistFile(path), "ファイルが存在しません\n");

	//ファイルを初期化する
	ErrorMessage(FUNC_NAME, fbxImporter->Initialize(path.c_str(), -1, fbxManager->GetIOSettings()) == false, "ファイルの初期化に失敗\n");

	//シーンオブジェクト生成
	FbxScene* fbxScene = FbxScene::Create(fbxManager, "scene");
	ErrorMessage(FUNC_NAME, fbxScene == nullptr, "FBXシーン生成に失敗\n");

	//シーンオブジェクトにfbxファイル内の情報を流し込む
	ErrorMessage(FUNC_NAME, fbxImporter->Import(fbxScene) == false, "FBXシーンへのFBXファイル情報流し込みに失敗\n");

	//シーン内のノードのポリゴンを全て三角形にする
	FbxGeometryConverter converter(fbxManager);

	//ポリゴンを三角形にする
	converter.Triangulate(fbxScene, true);
	//全FbxMeshをマテリアル単位で分割
	converter.SplitMeshesPerMaterial(fbxScene, true);

	//ノード探索（独自のノード配列に格納）
	std::vector<LoadFbxNode>loadFbxNodes;
	loadFbxNodes.reserve(fbxScene->GetNodeCount());
	ParseNodeRecursive(loadFbxNodes, fbxScene->GetRootNode());

	//情報の受け皿となるモデルオブジェクト生成
	result = std::make_shared<Model>(Dir, FileName);

	//スケルトン情報
	Skeleton skel;

	//先にボーンの情報取得（頂点に与える影響を取得するのに必要）
	for (auto itr = loadFbxNodes.begin(); itr != loadFbxNodes.end(); ++itr)
	{
		if (!itr->attribute)continue;	//Attributeを持たない
		if (itr->attribute->GetAttributeType() != FbxNodeAttribute::eSkeleton)continue;	//スケルトンノードではない

		//新規ボーン
		skel.bones.emplace_back();
		auto& newBone = skel.bones.back();

		//情報取得
		newBone.name = itr->name;
		newBone.invBindMat = XMMatrixInverse(nullptr, itr->transform);

		//親がいないならスルー
		if (!itr->parent)continue;

		//既に追加されているボーンから親を探す（親より子が先に存在することはない）
		for (int boneIdx = 0; boneIdx < skel.bones.size(); ++boneIdx)
		{
			if (skel.bones[boneIdx].name != itr->parent->name)continue;
			newBone.parent = boneIdx;
			break;
		}
	}

	//次にメッシュ情報読み込み
	for (auto itr = loadFbxNodes.begin(); itr != loadFbxNodes.end(); ++itr)
	{
		if (!itr->attribute)continue;	//Attributeを持たない
		if (itr->attribute->GetAttributeType() != FbxNodeAttribute::eMesh)continue;	//メッシュノードではない

		// 複数のAttributeからメッシュ読み込み（マテリアル単位で分割しているため必要）
		int meshCount = itr->fbxNode->GetNodeAttributeCount();
		for (int meshIdx = 0; meshIdx < meshCount; ++meshIdx)
		{
			FbxNodeAttribute* nodeAttribute = itr->fbxNode->GetNodeAttributeByIndex(meshIdx);
			if (nodeAttribute->GetAttributeType() != FbxNodeAttribute::EType::eMesh)continue;	//メッシュではない

			FbxMesh* fbxMesh = (FbxMesh*)nodeAttribute;

			//モデル用メッシュ生成
			ModelMesh mesh;
			mesh.mesh = std::make_shared<Mesh<ModelMesh::Vertex_Model>>();
			//メッシュ名セット
			mesh.mesh->name = fbxMesh->GetName();

			//メッシュ情報読み込み
			LoadFbxMesh(Dir, skel, mesh, fbxMesh);

			mesh.material->CreateBuff();
			mesh.mesh->CreateBuff();

			result->meshes.emplace_back(mesh);
		}
	}

	//アニメーションの読み込み
	int animStackCount = fbxImporter->GetAnimStackCount();	//アニメーションの数
	for (int animIdx = 0; animIdx < animStackCount; ++animIdx)
	{
		Skeleton::ModelAnimation animation;

		// AnimStack読み込み（AnimLayerの集合)
		FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(animIdx);
		std::string animName = animStack->GetName();	//アニメーション名取得

		//アニメーションが割り当てられている”部位”の数（AnimCurve の集合）
		int animLayersCount = animStack->GetMemberCount<FbxAnimLayer>();
		for (int i = 0; i < animLayersCount; ++i)
		{
			FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(i);
			std::string layerName = animLayer->GetName();

			for (auto itr = loadFbxNodes.begin(); itr != loadFbxNodes.end(); ++itr)
			{
				if (!itr->attribute)continue;	//Attributeを持たない
				if (itr->attribute->GetAttributeType() != FbxNodeAttribute::eSkeleton)continue;	//スケルトンノードではない

				LoadBoneAnim(*itr, animation, animLayer);
			}
		}
		skel.animations[animName] = animation;
	}

	skel.CreateBoneTree();
	result->skelton = std::make_shared<Skeleton>(skel);

	//SaveHSMModel(HSM_TAIL, result, fbxLastWriteTime);

	RegisterImportModel(Dir, FileName, result);

	return result;
}

void Importer::LoadGLTFMaterial(const MATERIAL_TEX_TYPE& Type, std::weak_ptr<Material> AttachMaterial, const Microsoft::glTF::Image& Img, const std::string& Dir, const Microsoft::glTF::GLTFResourceReader& Reader, const Microsoft::glTF::Document& Doc)
{
	auto material = AttachMaterial.lock();
	//テクスチャ画像ファイル読み込み
	if (!Img.uri.empty())
	{
		std::string path = Dir + Img.uri;
		material->texBuff[Type] = D3D12App::Instance()->GenerateTextureBuffer(path);
	}
	//テクスチャ画像がgltfに埋め込まれている
	else if (!Img.bufferViewId.empty())
	{
		auto imageBufferView = Doc.bufferViews.Get(Img.bufferViewId);
		auto imageData = Reader.ReadBinaryData<char>(Doc, imageBufferView);
		std::string path = "glTF - Load (" + Img.mimeType + ") - " + Img.name;
		material->texBuff[Type] = D3D12App::Instance()->GenerateTextureBuffer(imageData);
	}
}

#include<sstream>
std::shared_ptr<Model> Importer::LoadGLTFModel(const std::string& Dir, const std::string& FileName, const std::string& Ext)
{
	printf("glTFロード\nDir : %s , FileName : %s\n", Dir.c_str(), FileName.c_str());
	std::shared_ptr<Model>result = std::make_shared<Model>(Dir, FileName);

	auto modelFilePath = std::experimental::filesystem::path(Dir + FileName);
	if (modelFilePath.is_relative())
	{
		auto current = std::experimental::filesystem::current_path();
		current /= modelFilePath;
		current.swap(modelFilePath);
	}
	auto reader = std::make_unique<StreamReader>(modelFilePath.parent_path());
	auto stream = reader->GetInputStream(modelFilePath.filename().u8string());

	std::string manifest;

	auto MakePathExt = [](const std::string& ext)
	{
		return "." + ext;
	};

	std::unique_ptr<Microsoft::glTF::GLTFResourceReader> resourceReader;

	if (Ext == MakePathExt(Microsoft::glTF::GLTF_EXTENSION))
	{
		auto gltfResourceReader = std::make_unique<Microsoft::glTF::GLTFResourceReader>(std::move(reader));

		std::stringstream manifestStream;

		// Read the contents of the glTF file into a string using a std::stringstream
		manifestStream << stream->rdbuf();
		manifest = manifestStream.str();

		resourceReader = std::move(gltfResourceReader);
	}
	else if (Ext == MakePathExt(Microsoft::glTF::GLB_EXTENSION))
	{
		auto glbResourceReader = std::make_unique<Microsoft::glTF::GLBResourceReader>(std::move(reader), std::move(stream));

		manifest = glbResourceReader->GetJson(); // Get the manifest from the JSON chunk

		resourceReader = std::move(glbResourceReader);
	}

	KuroFunc::ErrorMessage(!resourceReader, "Importer", "LoadGLTFModel", "ファイルの拡張子が glb でも gltf でもありません\n");

	Microsoft::glTF::Document doc;
	try
	{
		doc = Microsoft::glTF::Deserialize(manifest);
	}
	catch (const Microsoft::glTF::GLTFException& ex)
	{
		std::stringstream ss;
		ss << "Microsoft::glTF::Deserialize failed: ";
		ss << ex.what();
		throw std::runtime_error(ss.str());
	}

	PrintDocumentInfo(doc);
	PrintResourceInfo(doc, *resourceReader);

	Skeleton skel;

	//ノード読み込み
	std::vector<Microsoft::glTF::Node>skinNodes;	//後に使うためスキンノードを格納する配列
	for (const auto& gltfNode : doc.nodes.Elements())
	{
		// ローカル変形行列の計算
		XMVECTOR rotation = { gltfNode.rotation.x, gltfNode.rotation.y, gltfNode.rotation.z, gltfNode.rotation.w };
		XMVECTOR scaling = { gltfNode.scale.x, gltfNode.scale.y, gltfNode.scale.z, 1.0f };
		XMVECTOR translation = { gltfNode.translation.x, gltfNode.translation.y, gltfNode.translation.z, 1.0f };

		XMMATRIX matScaling, matRotation, matTranslation;
		matScaling = XMMatrixScalingFromVector(scaling);
		matRotation = XMMatrixRotationQuaternion(rotation);
		matTranslation = XMMatrixTranslationFromVector(translation);

		auto nodeTransform = XMMatrixIdentity();
		nodeTransform *= matScaling * matRotation * matTranslation;

		//スキン情報
		if (!gltfNode.skinId.empty())
		{
			skinNodes.emplace_back(gltfNode);
			continue;
		}

		//ボーン → スキンの順番、その後に来るデータはアーマチュア？の情報でいらない情報ぽい
		if (!skinNodes.empty())break;

		//ボーン情報
		skel.bones.emplace_back();
		auto& bone = skel.bones.back();
		bone.name = gltfNode.name;
		for (auto& child : gltfNode.children)
		{
			auto childIdx = doc.nodes.GetIndex(child);
			skel.bones[childIdx].parent = skel.bones.size() - 1;
		}

		bone.invBindMat = XMMatrixInverse(nullptr, nodeTransform);

	}

	//スキン読み込み
	for (const auto& gltfSkin : doc.skins.Elements())
	{
		auto invMatAcc = doc.accessors.Get(gltfSkin.inverseBindMatricesAccessorId);

		auto data = resourceReader->ReadFloatData(doc, invMatAcc);
		for (int matIdx = 0; matIdx < invMatAcc.count; ++matIdx)
		{
			int offset = matIdx * 16;
			Matrix invBindMat = XMMatrixSet(
				data[offset], data[offset + 1], data[offset + 2], data[offset + 3],
				data[offset + 4], data[offset + 5], data[offset + 6], data[offset + 7],
				data[offset + 8], data[offset + 9], data[offset + 10], data[offset + 11],
				data[offset + 12], data[offset + 13], data[offset + 14], data[offset + 15]);

			int boneIdx = std::atoi(gltfSkin.jointIds[matIdx].c_str());
			skel.bones[boneIdx].invBindMat = invBindMat/* * skel.bones[boneIdx].invOffsetMat*/;
		}
	}

	//アニメーション
	for (const auto& gltfAnimNode : doc.animations.Elements())
	{
		std::string animName = gltfAnimNode.name;
		auto& modelAnim = skel.animations[animName];

		//アニメーションの詳細な情報を持つサンプラー（キーフレーム時刻リスト、補間形式、キーフレームに対応するアニメーションデータ）
		auto animSamplers = gltfAnimNode.samplers.Elements();

		//ボーン、サンプラー、パス（translation,rotation,scale）が割り当てられている
		for (const auto& animChannel : gltfAnimNode.channels.Elements())
		{
			const auto& sampler = animSamplers[gltfAnimNode.samplers.GetIndex(animChannel.samplerId)];
			const auto& boneNode = doc.nodes.Get(animChannel.target.nodeId);	//対応するボーンノード取得
			auto& boneAnim = modelAnim.boneAnim[boneNode.name];	//ボーン単位のアニメーション

			//アニメーション情報のターゲット（POS、ROTATE、SCALE）
			auto& path = animChannel.target.path;

			//補間方法（現状、LINERにしか対応しないので特に情報を格納しない）
			auto interpolation = sampler.interpolation;

			//開始 / 終了フレーム
			const auto& input = doc.accessors.Get(sampler.inputAccessorId);
			const int startFrame = input.min[0] * 60;	//単位が秒なので 60f / 1sec としてフレームに変換
			const int endFrame = input.max[0] * 60;	//単位が秒なので 60f / 1sec としてフレームに変換

			//キーフレーム情報
			auto keyFrames = resourceReader->ReadBinaryData<float>(doc, input);
			for (auto& keyFrame : keyFrames)keyFrame *= 60;	//単位が秒なので 60f / 1sec としてフレームに変換

			//具体的な値
			const auto& output = doc.accessors.Get(sampler.outputAccessorId);
			auto values = resourceReader->ReadBinaryData<float>(doc, output);

			ErrorMessage("LoadGLTFModel",
				path == Microsoft::glTF::TARGET_WEIGHTS || path == Microsoft::glTF::TARGET_UNKNOWN, "This anim's target path is unsupported.");

			//Rotation(Quaternion)
			if (path == Microsoft::glTF::TARGET_ROTATION)
			{
				boneAnim.rotateAnim.startFrame = startFrame;
				boneAnim.rotateAnim.endFrame = endFrame;

				for (int keyFrameIdx = 0; keyFrameIdx < keyFrames.size(); ++keyFrameIdx)
				{
					boneAnim.rotateAnim.keyFrames.emplace_back();
					auto& keyFrame = boneAnim.rotateAnim.keyFrames.back();
					keyFrame.frame = keyFrames[keyFrameIdx];
					int offset = keyFrameIdx * 4;	//インデックスのオフセット
					keyFrame.value = XMVectorSet(values[offset], values[offset + 1], values[offset + 2], values[offset + 3]);
				}
			}
			//TranslationかScale
			else
			{
				Animation<Vec3<float>>* animPtr = &boneAnim.posAnim;
				if (path == Microsoft::glTF::TARGET_SCALE) { animPtr = &boneAnim.scaleAnim; }

				animPtr->startFrame = startFrame;
				animPtr->endFrame = endFrame;

				for (int keyFrameIdx = 0; keyFrameIdx < keyFrames.size(); ++keyFrameIdx)
				{
					animPtr->keyFrames.emplace_back();
					auto& keyFrame = animPtr->keyFrames.back();
					keyFrame.frame = keyFrames[keyFrameIdx];
					int offset = keyFrameIdx * 3;	//インデックスのオフセット
					keyFrame.value = { values[offset], values[offset + 1], values[offset + 2] };
				}
			}
		}
	}

	skel.CreateBoneTree();
	result->skelton = std::make_shared<Skeleton>(skel);

	//マテリアル読み込み
	std::vector<std::shared_ptr<Material>>loadMaterials;
	for (auto& m : doc.materials.Elements())
	{
		auto material = std::make_shared<Material>();
		material->name = m.name;

		//PBR
		const auto baseColor = m.metallicRoughness.baseColorFactor;
		material->constData.pbr.baseColor.x = baseColor.r;
		material->constData.pbr.baseColor.y = baseColor.g;
		material->constData.pbr.baseColor.z = baseColor.b;
		material->constData.pbr.metalness = m.metallicRoughness.metallicFactor;
		material->constData.pbr.roughness = m.metallicRoughness.roughnessFactor;

		//カラーテクスチャ
		auto textureId = m.metallicRoughness.baseColorTexture.textureId;
		if (!textureId.empty())
		{
			auto& texture = doc.textures.Get(textureId);
			auto& image = doc.images.Get(texture.imageId);
			LoadGLTFMaterial(COLOR_TEX, material, image, Dir, *resourceReader, doc);
		}
		//エミッシブ
		textureId = m.emissiveTexture.textureId;
		if (!textureId.empty())
		{
			auto& texture = doc.textures.Get(textureId);
			auto& image = doc.images.Get(texture.imageId);
			LoadGLTFMaterial(EMISSIVE_TEX, material, image, Dir, *resourceReader, doc);
		}
		//ノーマルマップ
		textureId = m.normalTexture.textureId;
		if (!textureId.empty())
		{
			auto& texture = doc.textures.Get(textureId);
			auto& image = doc.images.Get(texture.imageId);
			LoadGLTFMaterial(NORMAL_TEX, material, image, Dir, *resourceReader, doc);
		}
		//ラフネス
		textureId = m.metallicRoughness.metallicRoughnessTexture.textureId;
		if (!textureId.empty())
		{
			auto& texture = doc.textures.Get(textureId);
			auto& image = doc.images.Get(texture.imageId);
			LoadGLTFMaterial(ROUGHNESS_TEX, material, image, Dir, *resourceReader, doc);
		}

		material->CreateBuff();
		loadMaterials.emplace_back(material);
	}

	for (const auto& glTFMesh : doc.meshes.Elements())
	{
		//メッシュIDを比較して対応するスキンノードを取得
		const Microsoft::glTF::Skin* gltfSkin = nullptr;
		const auto& meshId = glTFMesh.id;
		const auto& skinNode = std::find_if(skinNodes.begin(), skinNodes.end(), [meshId](Microsoft::glTF::Node& node) 
			{
				return node.meshId == meshId;
			});
		if (skinNode != skinNodes.end())gltfSkin = &doc.skins.Get(skinNode->skinId);

		const auto& meshName = glTFMesh.name;	//メッシュ名取得
		int primitiveNum = 0;	//プリミティブの数記録用

		//マテリアル単位で分けられている？
		for (const auto& gltfMeshPrimitive : glTFMesh.primitives)
		{
			//モデル用メッシュ生成
			ModelMesh mesh;
			mesh.mesh = std::make_shared<Mesh<ModelMesh::Vertex_Model>>();
			mesh.mesh->name = meshName + " - " + std::to_string(primitiveNum++);

			//頂点 & インデックス情報
			LoadGLTFPrimitive(mesh, gltfMeshPrimitive, gltfSkin, *resourceReader, doc);

			if (doc.materials.Has(gltfMeshPrimitive.materialId))
			{
				int materialIdx = int(doc.materials.GetIndex(gltfMeshPrimitive.materialId));
				mesh.material = loadMaterials[materialIdx];
			}
			else
			{
				mesh.material = std::make_shared<Material>();
			}

			mesh.mesh->CreateBuff();
			result->meshes.emplace_back(mesh);
		}
	}

	return result;
}

std::shared_ptr<Model> Importer::LoadModel(const std::string& Dir, const std::string& FileName)
{
	//ファイルが存在しているか確認
	ErrorMessage("LoadModel", !KuroFunc::ExistFile(Dir + FileName), Dir + FileName + " wasn't found.\n");

	//拡張子取得
	const auto ext = "." + KuroFunc::GetExtension(FileName);

	if (ext == ".fbx" || ext == ".FBX")
	{
		return LoadFBXModel(Dir, FileName, ext);
	}
	else if (ext == ".glb" || ext == ".gltf")
	{
		return LoadGLTFModel(Dir, FileName, ext);
	}
	else
	{
		ErrorMessage("LoadModel", true, "This format is valid.");
	}
	return std::shared_ptr<Model>();
}