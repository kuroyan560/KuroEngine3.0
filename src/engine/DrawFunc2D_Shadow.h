#pragma once
#include"D3D12Data.h"
#include<memory>
#include<vector>
#include"Vec.h"
#include"Color.h"

class LightManager;

class DrawFunc2D_Shadow
{
	//視点座標用の定数バッファ
	static std::shared_ptr<ConstantBuffer>s_eyePosBuff;
	//白のベタ塗りテクスチャ
	static std::shared_ptr<TextureBuffer>s_defaultTex;
	// (0,0,-1) のベタ塗りノーマルマップ
	static std::shared_ptr<TextureBuffer>s_defaultNormalMap;
	//黒のベタ塗りテクスチャ
	static std::shared_ptr<TextureBuffer>s_defaultEmissiveMap;

	//DrawExtendGraph
	static int s_drawExtendGraphCount;

	//DrawRotaGraph
	static int s_drawRotaGraphCount;

	static void StaticInit();
public:
	static void SetEyePos(Vec3<float> EyePos);

public:
	//呼び出しカウントリセット
	static void CountReset()
	{
		s_drawExtendGraphCount = 0;
		s_drawRotaGraphCount = 0;
	}

	/// <summary>
	/// 拡大縮小描画
	/// </summary>
	/// <param name="LeftUpPos">矩形の左上座標</param>
	/// <param name="RightBottomPos">矩形の右上座標</param>
	/// <param name="Tex">テクスチャ</param>
	/// <param name="NormalMap">法線マップ</param>
	/// <param name="EmissiveMap">放射マップ(光源材質)</param>
	/// <param name="SpriteDepth">陰影決定に利用するZ値</param>
	/// <param name="Miror">反転フラグ</param>
	/// <param name="Diffuse">Diffuse影響度</param>
	/// <param name="Specular">Specular影響度</param>
	/// <param name="Lim">Lim影響度</param>
	static void DrawExtendGraph2D(LightManager& LigManager,
		const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos,
		const std::shared_ptr<TextureBuffer>& Tex, const std::shared_ptr<TextureBuffer>& NormalMap = nullptr,
		const std::shared_ptr<TextureBuffer>& EmissiveMap = nullptr, const float& SpriteDepth = 0.0f, 
		const Vec2<bool>& Miror = { false,false },
		const float& Diffuse = 1.0f, const float& Specular = 1.0f, const float& Lim = 1.0f);

	/// <summary>
	/// ２D画像回転描画(陰影)
	/// </summary>
	/// <param name="LigManager">ライト情報</param>
	/// <param name="Center">中心座標</param>
	/// <param name="ExtRate">拡大率</param>
	/// <param name="Radian">回転角度</param>
	/// <param name="Tex">テクスチャ</param>
	/// <param name="NormalMap">法線マップ</param>
	/// <param name="EmissiveMap">放射マップ(光源材質)</param>
	/// <param name="SpriteDepth">陰影決定に利用するZ値</param>
	/// <param name="RotaCenterUV">回転中心UV( 0.0 ~ 1.0 )</param>
	/// <param name="Miror">反転フラグ</param>
	/// <param name="Diffuse">Diffuse影響度</param>
	/// <param name="Specular">Specular影響度</param>
	/// <param name="Lim">Lim影響度</param>
	static void DrawRotaGraph2D(LightManager& LigManager,
		const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian,
		const std::shared_ptr<TextureBuffer>& Tex, const std::shared_ptr<TextureBuffer>& NormalMap = nullptr,
		const std::shared_ptr<TextureBuffer>& EmissiveMap = nullptr, const float& SpriteDepth = 0.0f,
		const Vec2<float>& RotaCenterUV = { 0.5f,0.5f }, const Vec2<bool>& Miror = { false,false },
		const float& Diffuse = 1.0f, const float& Specular = 1.0f, const float& Lim = 1.0f);
};