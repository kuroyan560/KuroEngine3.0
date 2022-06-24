#pragma once
#include"Color.h"
#include"D3D12Data.h"
#include<vector>
#include"Transform.h"
#include<memory>
#include"Camera.h"
#include"Object.h"

class Model;
class LightManager;
class CubeMap;
class ModelAnimator;

class DrawFunc3D
{
	//DrawLine
	static int DRAW_LINE_COUNT;
	//DrawNonShadingModel
	static int DRAW_NON_SHADING_COUNT;
	//DrawADSShadingModel
	static int DRAW_ADS_SHADING_COUNT;
	//DrawPBRShadingModel
	static int DRAW_PBR_SHADING_COUNT;
	//DrawToonModel
	static int DRAW_TOON_COUNT;	
	//DrawShadowFallModel
	static int DRAW_SHADOW_FALL_COUNT;

public:
	//呼び出しカウントリセット
	static void CountReset()
	{
		DRAW_LINE_COUNT = 0;
		DRAW_NON_SHADING_COUNT = 0;
		DRAW_ADS_SHADING_COUNT = 0;
		DRAW_PBR_SHADING_COUNT = 0;
		DRAW_TOON_COUNT = 0;
		DRAW_SHADOW_FALL_COUNT = 0;
	}

	//線描画
	static void DrawLine(Camera& Cam, const Vec3<float>& From, const Vec3<float>& To, const Color& LineColor, const float& Thickness, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	//通常描画
	static void DrawNonShadingModel(const std::weak_ptr<Model>Model, Transform& Transform, Camera& Camera, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawNonShadingModel(const std::weak_ptr<ModelObject>ModelObject, Camera& Camera, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawNonShadingModel(obj->model, obj->transform, Camera, BlendMode);
	}
	//影つき描画
	static void DrawADSShadingModel(LightManager& LigManager, const std::weak_ptr<Model>Model, Transform& Transform, Camera& Cam, std::shared_ptr<ModelAnimator> Animator = nullptr, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawADSShadingModel(LightManager& LigManager, const std::weak_ptr<ModelObject>ModelObject, Camera& Cam, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawADSShadingModel(LigManager, obj->model, obj->transform, Cam, obj->animator, BlendMode);
	}
	//影つき描画(PBR)
	static void DrawPBRShadingModel(LightManager& LigManager, const std::weak_ptr<Model>Model, Transform& Transform, Camera& Cam, std::shared_ptr<ModelAnimator> Animator = nullptr, std::shared_ptr<CubeMap>AttachCubeMap = nullptr, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawPBRShadingModel(LightManager& LigManager, const std::weak_ptr<ModelObject>ModelObject, Camera& Cam, std::shared_ptr<CubeMap>AttachCubeMap = nullptr, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawPBRShadingModel(LigManager, obj->model, obj->transform, Cam, obj->animator, AttachCubeMap, BlendMode);
	}
	//トゥーンシェーディング
	static void DrawToonModel(const std::weak_ptr<TextureBuffer>ToonTex, LightManager& LigManager, const std::weak_ptr<Model>Model, Transform& Transform, Camera& Cam, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawToonModel(const std::weak_ptr<TextureBuffer>ToonTex, LightManager& LigManager, const std::weak_ptr<ModelObject>ModelObject, Camera& Cam, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawToonModel(ToonTex, LigManager, obj->model, obj->transform, Cam, BlendMode);
	}
};