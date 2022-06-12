#include "ModelAnimator.h"
#include"KuroEngine.h"
#include"Model.h"

void ModelAnimator::BoneMatrixRecursive(const int& BoneIdx, const Matrix& ParentMatrix)
{
	auto skel = attachSkelton.lock();
	const auto& bone = skel->bones[BoneIdx];

	//親の行列を適用
	boneMatricies[BoneIdx] *= ParentMatrix;

	//子を呼び出して再帰的に計算
	for (auto& child : bone.children)
	{
		BoneMatrixRecursive(child, boneMatricies[BoneIdx]);
	}
}

ModelAnimator::ModelAnimator(std::weak_ptr<Model> Model)
{
	Attach(Model);
}

void ModelAnimator::Attach(std::weak_ptr<Model> Model)
{
	auto model = Model.lock();
	auto skel = model->skelton;
	KuroFunc::ErrorMessage(MAX_BONE_NUM < skel->bones.size(), "ModelAnimator", "AttachSkeleton", "The bone's number is over than limit.");

	//バッファ未生成
	if (!boneBuff)
	{
		boneBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), MAX_BONE_NUM);
	}
	
	//バッファのリネーム
	boneBuff->GetResource()->SetName((L"BoneMatricies - " + KuroFunc::GetWideStrFromStr(model->header.GetModelName())).c_str());

	Reset();

	//スケルトンをアタッチ
	attachSkelton = skel;
}

void ModelAnimator::Reset()
{
	//単位行列で埋めてリセット
	std::array<Matrix, MAX_BONE_NUM>initMat;
	initMat.fill(XMMatrixIdentity());
	boneBuff->Mapping(initMat.data());

	//再生中アニメーション名リセット
	playAnimations.clear();
}

void ModelAnimator::Play(const std::string& AnimationName, const bool& Loop)
{
	auto skel = attachSkelton.lock();
	KuroFunc::ErrorMessage(!skel, "ModelAnimator", "Play", "Any skeleton doesn't be attached.");
	KuroFunc::ErrorMessage(!skel->animations.contains(AnimationName), "ModelAnimator", "Play", "That animation wasn't found.");

	//既に再生中か調べる
	auto already = std::find_if(playAnimations.begin(), playAnimations.end(), [AnimationName](PlayAnimation& Anim)
		{ return !Anim.name.compare(AnimationName); } );
	//再生中ならリセットしておわり
	if (already != playAnimations.end())
	{
		already->past = 0;
		already->loop = Loop;
		return;
	}

	//新規アニメーション追加
	playAnimations.emplace_back(AnimationName, Loop);
}

void ModelAnimator::Update()
{
	auto skel = attachSkelton.lock();
	if (!skel)return;	//スケルトンがアタッチされていない
	if (playAnimations.empty())return;	//アニメーション再生中でない

	//単位行列で埋めてリセット
	std::fill(boneMatricies.begin(), boneMatricies.end(), XMMatrixIdentity());

	//再生中のアニメーション
	for (auto& playAnim : playAnimations)
	{
		//アニメーション情報取得
		const auto& anim = skel->animations[playAnim.name];

		//アニメーションが終了しているかのフラグ
		bool animFinish = true;

		//ボーン単位のアニメーション
		for (auto& boneAnim : anim.boneAnim)
		{
			int boneIdx = skel->boneIdxTable[boneAnim.first];

			//ボーンアニメーションから行列取得
			auto boneAnimMat = boneAnim.second.GetMatrix(playAnim.past, animFinish ? &animFinish : nullptr);
			boneMatricies[boneIdx] = boneAnimMat;
			//オフセット行列
			boneMatricies[boneIdx] *= skel->bones[boneIdx].invOffsetMat;
		}

		//全ての親は一番後ろののボーン、再帰的に計算して親のトランスフォームを適用
		BoneMatrixRecursive(skel->bones.size() - 1, XMMatrixIdentity());

		//フレーム経過
		playAnim.past++;
		//アニメーションの終了情報記録
		playAnim.finish = animFinish;
	}

	//終了しているアニメーションを調べる
	for (auto itr = playAnimations.begin(); itr != playAnimations.end();)
	{
		//アニメーション終了していないならスルー
		if (!itr->finish) { ++itr; continue; }
		//ループフラグが立っているなら経過フレームをリセットしてスルー
		if (itr->loop) { itr->past = 0; ++itr; continue; }
		//終了しているので削除
		itr = playAnimations.erase(itr);
	}

	//バッファにデータ転送
	boneBuff->Mapping(boneMatricies.data());
}
