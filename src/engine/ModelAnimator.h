#pragma once
#include<memory>
#include<string>
#include<list>
#include<array>
#include"KuroMath.h"
#include"Skeleton.h"
class Model;
class ConstantBuffer;

class ModelAnimator
{
	static const int MAX_BONE_NUM = 256;

	//対応するスケルトンの参照
	std::weak_ptr<Skeleton>attachSkelton;
	//ボーンのローカル行列
	std::shared_ptr<ConstantBuffer>boneBuff;
	//ボーン行列
	std::array<Matrix, MAX_BONE_NUM>boneMatricies;

	struct PlayAnimation
	{
		std::string name;	//アニメーション名
		float past = 0;	//経過フレーム(スローモーションなど考慮して浮動小数点)
		bool loop;
		bool finish = false;

		PlayAnimation(const std::string& Name, const bool& Loop) :name(Name), loop(Loop) {}
	};

	std::list<PlayAnimation>playAnimations;

	void BoneMatrixRecursive(const int& BoneIdx, const Matrix& ParentMatrix, const int& Past, bool* Finish, Skeleton::ModelAnimation& Anim);

public:
	float speed = 1.0f;
	bool stop = false;

	ModelAnimator() {}
	ModelAnimator(std::weak_ptr<Model>Model);
	void Attach(std::weak_ptr<Model>Model);

	void Reset();

	//アニメーション再生
	void Play(const std::string& AnimationName, const bool& Loop);
	void Update();
	const std::shared_ptr<ConstantBuffer>& GetBoneMatBuff() { return boneBuff; }
};