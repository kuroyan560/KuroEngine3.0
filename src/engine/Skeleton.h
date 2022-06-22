#pragma once
#include<string>
#include"Vec.h"
#include"KuroMath.h"
#include<map>
#include"Animation.h"
#include<array>

class Bone
{
public:
	static size_t GetSizeWithOutName()
	{
		return sizeof(char) + sizeof(int) + sizeof(Vec3<float>) + sizeof(Matrix);
	}

	std::string name;
	char parent = -1;	//親ボーン
	std::vector<char>children;	//子ボーン
	int transLayer = 0;	//変形階層
	Vec3<float> pos = { 0.0f,0.0f,0.0f };
	Matrix invOffsetMat = DirectX::XMMatrixIdentity();
};

class Skeleton
{
public:
	struct BoneAnimation
	{
	private:
		enum ANIM_TYPE { TRANSLATION, ROTATION, SCALING };
		Matrix CalculateMat(const ANIM_TYPE& Type, const float& Frame, bool& FinishFlg)const;

	public:
		Animation<Vec3<float>>posAnim;
		Animation<XMVECTOR>rotateAnim;
		Animation<Vec3<float>>scaleAnim;
		Matrix GetMatrix(const float& Frame, bool* FinishFlg = nullptr)const;
	};
	struct ModelAnimation
	{
		std::map<std::string, BoneAnimation>boneAnim;	//ボーン単位のアニメーション
	};

	std::vector<Bone>bones;
	std::map<std::string, int>boneIdxTable;
	/*
		アニメーション情報（Skeletonがアニメーションを行う訳では無い。Animatorからの参照用）
		キーは アニメーション名
	*/
	std::map<std::string, ModelAnimation>animations;
	void CreateBoneTree();
	int GetIndex(const std::string& BoneName);
};

