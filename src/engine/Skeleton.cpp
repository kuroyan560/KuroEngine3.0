#include "Skeleton.h"
#include"KuroFunc.h"

const std::string Skeleton::DEFINE_PARENT_BONE_NAME = "Defined_Parent";

void Skeleton::CreateBoneTree()
{
	//ボーンがないなら無視
	if (bones.empty())return;

	//全ての親となるボーンを追加
	int parentBoneIdx = bones.size();
	bones.emplace_back();
	bones.back().name = "Defined_Parent";

	//ボーンノードマップを作る
	for (int idx = 0; idx < bones.size(); ++idx)
	{
		auto& bone = bones[idx];
		boneIdxTable[bone.name] = idx;

		if (bone.parent != -1)continue;	//既に親がいる
		if (idx == parentBoneIdx)continue;	//自信が全ての親
		bone.parent = parentBoneIdx;
	}

	//親子関係構築
	for (int i = 0; i < bones.size(); ++i)
	{
		auto& bone = bones[i];
		//親インデックスをチェック(ありえない番号ならとばす)
		if (bone.parent < 0 || bones.size() < bone.parent)
		{
			continue;
		}
		bones[bone.parent].children.emplace_back(i);
	}
}

int Skeleton::GetIndex(const std::string& BoneName)
{
	KuroFunc::ErrorMessage(bones.empty(), "Skeleton", "GetIndex", "ボーン情報がありません\n");
	KuroFunc::ErrorMessage(!boneIdxTable.contains(BoneName), "Skeleton", "GetIndex", "存在しないボーンが参照されました (" + BoneName + ")\n");
	return boneIdxTable[BoneName];
}