#pragma once
#include<memory>
#include<string>
#include<list>
class ConstantBuffer;
class Skeleton;
class Model;
class ModelAnimator
{
	static const int MAX_BONE_NUM = 256;

	//対応するスケルトンの参照
	std::weak_ptr<Skeleton>attachSkelton;
	//ボーンのローカル行列
	std::shared_ptr<ConstantBuffer>boneBuff;

	struct PlayAnimation
	{
		std::string name;	//アニメーション名
		float past = 0;	//経過フレーム(スローモーションなど考慮して浮動小数点)
		bool loop;
		bool finish = false;

		PlayAnimation(const std::string& Name, const bool& Loop) :name(Name), loop(Loop) {}
	};

	std::list<PlayAnimation>playAnimations;

public:
	ModelAnimator() {}
	ModelAnimator(std::weak_ptr<Model>Model);
	void Attach(std::weak_ptr<Model>Model);

	void Reset();
	void Play(const std::string& AnimationName, const bool& Loop);
	void Update();
	const std::shared_ptr<ConstantBuffer>& GetBoneMatBuff() { return boneBuff; }
};