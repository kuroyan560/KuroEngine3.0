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

Matrix Skeleton::BoneAnimation::CalculateMat(const ANIM_TYPE& Type, const float& Frame, bool& FinishFlg)const
{
	Matrix result = XMMatrixIdentity();

	//Translation
	if (Type == TRANSLATION)
	{
		//キーフレーム情報なし
		if (posAnim.keyFrames.empty())return XMMatrixIdentity();

		//結果の格納先
		Vec3<float>translation;

		if (Frame < posAnim.startFrame)translation = posAnim.keyFrames.front().value;		//範囲外：一番手前を採用
		else if (posAnim.endFrame < Frame)translation = posAnim.keyFrames.back().value;	//範囲外：一番最後を採用
		else
		{
			FinishFlg = false;	//フレーム範囲内なのでアニメーションは終了していない

			const KeyFrame<Vec3<float>>* firstKey = nullptr;
			const KeyFrame<Vec3<float>>* secondKey = nullptr;
			for (auto& key : posAnim.keyFrames)
			{
				//同じフレーム数の物があったらそれを採用
				if (key.frame == Frame)
				{
					translation = key.value;
					break;
				}

				if (key.frame < Frame)firstKey = &key;	//補間の開始キーフレーム
				if (secondKey == nullptr && Frame < key.frame)secondKey = &key;	//補間の終了キーフレーム

				//補間の情報が揃ったので線形補間してそれを採用
				if (firstKey != nullptr && secondKey != nullptr)
				{
					translation = KuroMath::Lerp(firstKey->value, secondKey->value, (Frame - firstKey->frame) / (secondKey->frame - firstKey->frame));
					break;
				}
			}
		}

		//結果を行列に変換
		result = XMMatrixTranslation(translation.x, translation.y, translation.z);
	}
	//Rotation
	else if (Type == ROTATION)
	{
		//キーフレーム情報なし
		if (rotateAnim.keyFrames.empty())return XMMatrixIdentity();

		//結果の格納先
		XMVECTOR rotation;

		if (Frame < rotateAnim.startFrame)rotation = rotateAnim.keyFrames.front().value;		//範囲外：一番手前を採用
		else if (rotateAnim.endFrame < Frame)rotation = rotateAnim.keyFrames.back().value;	//範囲外：一番最後を採用
		else
		{
			FinishFlg = false;	//フレーム範囲内なのでアニメーションは終了していない

			const KeyFrame<XMVECTOR>* firstKey = nullptr;
			const KeyFrame<XMVECTOR>* secondKey = nullptr;
			for (auto& key : rotateAnim.keyFrames)
			{
				//同じフレーム数の物があったらそれを採用
				if (key.frame == Frame)
				{
					rotation = key.value;
					break;
				}

				if (key.frame < Frame)firstKey = &key;	//補間の開始キーフレーム
				if (secondKey == nullptr && Frame < key.frame)secondKey = &key;	//補間の終了キーフレーム

				//補間の情報が揃ったので線形補間してそれを採用
				if (firstKey != nullptr && secondKey != nullptr)
				{
					rotation = XMQuaternionSlerp(firstKey->value, secondKey->value, (Frame - firstKey->frame) / (secondKey->frame - firstKey->frame));
					break;
				}
			}
		}
		//結果を行列に変換
		result = XMMatrixRotationQuaternion(rotation);
	}
	//Scaling
	else if (Type == SCALING)
	{
		//キーフレーム情報なし
		if (scaleAnim.keyFrames.empty())return XMMatrixIdentity();

		//結果の格納先
		Vec3<float>scale;

		if (Frame < scaleAnim.startFrame)scale = scaleAnim.keyFrames.front().value;		//範囲外：一番手前を採用
		else if (scaleAnim.endFrame < Frame)scale = scaleAnim.keyFrames.back().value;	//範囲外：一番最後を採用
		else
		{
			FinishFlg = false;	//フレーム範囲内なのでアニメーションは終了していない

			const KeyFrame<Vec3<float>>* firstKey = nullptr;
			const KeyFrame<Vec3<float>>* secondKey = nullptr;
			for (auto& key : scaleAnim.keyFrames)
			{
				//同じフレーム数の物があったらそれを採用
				if (key.frame == Frame)
				{
					scale = key.value;
					break;
				}

				if (key.frame < Frame)firstKey = &key;	//補間の開始キーフレーム
				if (secondKey == nullptr && Frame < key.frame)secondKey = &key;	//補間の終了キーフレーム

				//補間の情報が揃ったので線形補間してそれを採用
				if (firstKey != nullptr && secondKey != nullptr)
				{
					scale = KuroMath::Lerp(firstKey->value, secondKey->value, (Frame - firstKey->frame) / (secondKey->frame - firstKey->frame));
					break;
				}
			}
		}
		//結果を行列に変換
		result = XMMatrixScaling(scale.x, scale.y, scale.z);
	}
	else assert(0);

	return result;
}

Matrix Skeleton::BoneAnimation::GetMatrix(const float& Frame, bool* FinishFlg)const
{
	Matrix result = XMMatrixIdentity();
	bool finish = true;

	Matrix translation = CalculateMat(TRANSLATION, Frame, finish);
	Matrix rotation = CalculateMat(ROTATION, Frame, finish);
	Matrix scaling = CalculateMat(SCALING, Frame, finish);

	result *= translation * rotation * scaling;

	if (FinishFlg)*FinishFlg = finish;

	return result;
}
