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
	static std::shared_ptr<ConstantBuffer>EYE_POS_BUFF;
	//白のベタ塗りテクスチャ
	static std::shared_ptr<TextureBuffer>DEFAULT_TEX;
	// (0,0,-1) のベタ塗りノーマルマップ
	static std::shared_ptr<TextureBuffer>DEFAULT_NORMAL_MAP;
	//黒のベタ塗りテクスチャ
	static std::shared_ptr<TextureBuffer>DEFAULT_EMISSIVE_MAP;

	//DrawExtendGraph
	static int DRAW_EXTEND_GRAPH_COUNT;

	//DrawRotaGraph
	static int DRAW_ROTA_GRAPH_COUNT;

	static void StaticInit();
public:
	static void SetEyePos(Vec3<float> EyePos);

public:
	//呼び出しカウントリセット
	static void CountReset()
	{
		DRAW_EXTEND_GRAPH_COUNT = 0;
		DRAW_ROTA_GRAPH_COUNT = 0;
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