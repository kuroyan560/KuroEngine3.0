#pragma once
#include"Vec.h"
#include<array>

#include<memory>
class TextureBuffer;

class HitEffect
{
private:
	static const int MAX_NUM = 300;
	static std::array<HitEffect, MAX_NUM>INSTANCES;

public:
	HitEffect() {}

	static void Generate(const Vec2<float>& Pos);
	static void Init();
	static void Update();
	static void Draw(std::shared_ptr<TextureBuffer>&Noise, std::shared_ptr<TextureBuffer>& Noise2);
	static HitEffect& GetInstance(const int& Idx) { return INSTANCES[Idx]; }

public:
	char isActive = 0;
	Vec2<float>pos = { 0,0 };			//座標
	float scale = 1.0f;						//描画スケール
	float rotate = 0.0f;					//回転（ラジアン）
	float alpha = 1.0f;					//アルファ値
	float lifeTimer = 0.0f;				//寿命計測用タイマー（1.0fで終了）
	int lifeSpan = 60;						//寿命（単位：フレーム
	float blur = 0.0f;						//ブラーの強さ
	float uvRadiusOffset = 0.0f;		//中心から外側にかけてのUVアニメーション用
	float circleThickness = 0.125f;	//ノイズをかける前の元となる円画像の太さ
	float circleRadius = 0.25f;			//ノイズをかける前の元となる円画像の半径
};
