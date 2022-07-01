#pragma once
#include"KuroEngine.h"
#include"LightManager.h"
#include"Player.h"
#include"GameManager.h"
#include"ShadowMapDevice.h"
#include"HitEffect.h"

class Model;
class ModelObject;
class GaussianBlur;
class Enemy;

class GameScene : public BaseScene
{
	ShadowMapDevice shadowMapDevice;
	std::shared_ptr<ModelObject>floorModel;

	LightManager ligMgr;
	Light::Direction dirLig;
	Light::HemiSphere hemiLig;
	Light::Point ptLig;
	
	Player player;

	struct Noise
	{
		std::shared_ptr<TextureBuffer>noise;
		int split = 8;
		int octaves = 5;
		float frequency = 1.0f;
		float persistance = 0.5f;
	};
	Noise noises[2];
	Vec2<int>noiseSize = { 256 * 2,256 * 2 };

	void NoiseGenerate();

public:
	GameScene();
	void OnInitialize()override;
	void OnUpdate()override;
	void OnDraw()override;
	void OnImguiDebug()override;
	void OnFinalize()override;
};