#pragma once
#include"Vec.h"
#include<array>

#include<memory>
class TextureBuffer;
class Camera;

class HitEffect
{
private:
	static const int MAX_NUM = 300;
	static std::array<HitEffect, MAX_NUM>s_instanceArray;

public:
	HitEffect() {}

	static void Generate(const Vec3<float>& Pos);
	static void Init();
	static void Update();
	static void Draw(Camera& Cam);
	static HitEffect& GetInstance(const int& Idx) { return s_instanceArray[Idx]; }

public:
	char m_isActive = 0;
	Vec3<float>m_pos = { 0,0,0 };			//座標
	float m_scale = 1.0f;						//描画スケール
	float m_rotate = 0.0f;					//回転（ラジアン）
	float m_alpha = 1.0f;					//アルファ値
	float m_lifeTimer = 0.0f;				//寿命計測用タイマー（1.0fで終了）
	int m_lifeSpan = 60;						//寿命（単位：フレーム
	float m_blur = 0.0f;						//ブラーの強さ
	float m_uvRadiusOffset = 0.0f;		//中心から外側にかけてのUVアニメーション用
	float m_circleThickness = 0.125f;	//ノイズをかける前の元となる円画像の太さ
	float m_circleRadius = 0.25f;			//ノイズをかける前の元となる円画像の半径
};
