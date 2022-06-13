#pragma once
#include<memory>
#include<string>
#include<list>
#include<vector>
#include"KuroMath.h"
class Skeleton;
class Model;
class ModelAnimator
{
private:
	//対応するスケルトンの参照
	std::weak_ptr<Skeleton>attachSkelton;
	//ボーン行列
	std::vector<Matrix>boneMatricies;

	struct PlayAnimation
	{
		std::string name;	//アニメーション名
		float past = 0;	//経過フレーム(スローモーションなど考慮して浮動小数点)
		bool loop;
		bool finish = false;

		PlayAnimation(const std::string& Name, const bool& Loop) :name(Name), loop(Loop) {}
	};

	std::list<PlayAnimation>playAnimations;

	void BoneMatrixRecursive(const int& BoneIdx, const Matrix& ParentMatrix);

public:
	ModelAnimator() {}
	ModelAnimator(std::weak_ptr<Model>Model);
	void Attach(std::weak_ptr<Model>Model);

	void Reset();
	void Play(const std::string& AnimationName, const bool& Loop);
	void Update();

	const std::vector<Matrix>& GetBoneMatricies() { return boneMatricies; }
};