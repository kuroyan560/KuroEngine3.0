#include "Skeleton.h"
#include"KuroFunc.h"

void Skeleton::CreateBoneTree()
{
	//ボーンがないなら無視
	if (bones.empty())return;

	//全ての親となるボーンを追加
	int parentBoneIdx = bones.size();
	bones.emplace_back();
	bones.back().name = "Parent";

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

Matrix Skeleton::BoneAnimation::GetMatrix(const float& Frame, bool* FinishFlg)const
{
	Matrix result = XMMatrixIdentity();
	bool finish = true;

	std::array<float, ANIM_IDX_NUM>getValue = { 0 };
	for (int i = 0; i < ANIM_IDX_NUM; ++i)
	{
		if (anims[i].keyFrames.empty())continue;		//キーフレーム情報なし
		if (Frame < anims[i].startFrame)
		{
			getValue[i] = anims[i].keyFrames.front().value;	//範囲外：一番手前を採用
			continue;
		}
		if (anims[i].endFrame < Frame)
		{
			getValue[i] = anims[i].keyFrames.back().value;	//範囲外：一番最後を採用
			continue;
		}

		//範囲内なのでアニメーション終了していない
		finish = false;

		const KeyFrame* firstKeyFrame = nullptr;
		const KeyFrame* secondKeyFrame = nullptr;
		for (auto& key : anims[i].keyFrames)
		{
			//同じフレーム数の物があったらそれを採用
			if (key.frame == Frame)
			{
				getValue[i] = key.value;
				break;
			}

			if (key.frame < Frame)firstKeyFrame = &key;	//補間の開始キーフレーム
			if (secondKeyFrame == nullptr && Frame < key.frame)secondKeyFrame = &key;	//補間の終了キーフレーム

			//補間の情報が揃ったので線形補間してそれを採用
			if (firstKeyFrame != nullptr && secondKeyFrame != nullptr)
			{
				getValue[i] = KuroMath::Liner(firstKeyFrame->value, secondKeyFrame->value, (Frame - firstKeyFrame->frame) / (secondKeyFrame->frame - firstKeyFrame->frame));
				break;
			}
		}
	}

	result *= XMMatrixTranslation(getValue[POS_X], getValue[POS_Y], getValue[POS_Z])
		* XMMatrixRotationQuaternion(XMVectorSet(getValue[ROTATE_X], getValue[ROTATE_Y], getValue[ROTATE_Z], getValue[ROTATE_W]))
		* XMMatrixScaling(getValue[SCALE_X], getValue[SCALE_Y], getValue[SCALE_Z]);

	if (FinishFlg)*FinishFlg = finish;

	return result;
}
