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
	static int s_drawLineCount;
	//DrawNonShadingModel
	static int s_drawNonShadingCount;
	//DrawADSShadingModel
	static int s_drawAdsShadingCount;
	//DrawPBRShadingModel
	static int s_drawPbrShadingCount;
	//DrawToonModel
	static int s_drawToonCount;	
	//DrawShadowFallModel
	static int s_drawShadowFallCount;

public:
	//呼び出しカウントリセット
	static void CountReset()
	{
		s_drawLineCount = 0;
		s_drawNonShadingCount = 0;
		s_drawAdsShadingCount = 0;
		s_drawPbrShadingCount = 0;
		s_drawToonCount = 0;
		s_drawShadowFallCount = 0;
	}

	//線描画
	static void DrawLine(Camera& Cam, const Vec3<float>& From, const Vec3<float>& To, const Color& LineColor, const float& Thickness, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	//通常描画
	static void DrawNonShadingModel(const std::weak_ptr<Model>Model, Transform& Transform, Camera& Camera, std::shared_ptr<ModelAnimator> Animator = nullptr, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawNonShadingModel(const std::weak_ptr<ModelObject>ModelObject, Camera& Camera, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawNonShadingModel(obj->m_model, obj->m_transform, Camera, obj->m_animator, BlendMode);
	}
	//影つき描画
	static void DrawADSShadingModel(LightManager& LigManager, const std::weak_ptr<Model>Model, Transform& Transform, Camera& Cam, std::shared_ptr<ModelAnimator> Animator = nullptr, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawADSShadingModel(LightManager& LigManager, const std::weak_ptr<ModelObject>ModelObject, Camera& Cam, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawADSShadingModel(LigManager, obj->m_model, obj->m_transform, Cam, obj->m_animator, BlendMode);
	}
	//影つき描画(PBR)
	static void DrawPBRShadingModel(LightManager& LigManager, const std::weak_ptr<Model>Model, Transform& Transform, Camera& Cam, std::shared_ptr<ModelAnimator> Animator = nullptr, std::shared_ptr<CubeMap>AttachCubeMap = nullptr, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawPBRShadingModel(LightManager& LigManager, const std::weak_ptr<ModelObject>ModelObject, Camera& Cam, std::shared_ptr<CubeMap>AttachCubeMap = nullptr, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawPBRShadingModel(LigManager, obj->m_model, obj->m_transform, Cam, obj->m_animator, AttachCubeMap, BlendMode);
	}
	//トゥーンシェーディング
	static void DrawToonModel(const std::weak_ptr<TextureBuffer>ToonTex, LightManager& LigManager, const std::weak_ptr<Model>Model, Transform& Transform, Camera& Cam, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans);
	static void DrawToonModel(const std::weak_ptr<TextureBuffer>ToonTex, LightManager& LigManager, const std::weak_ptr<ModelObject>ModelObject, Camera& Cam, const AlphaBlendMode& BlendMode = AlphaBlendMode_Trans)
	{
		auto obj = ModelObject.lock();
		DrawToonModel(ToonTex, LigManager, obj->m_model, obj->m_transform, Cam, BlendMode);
	}
};