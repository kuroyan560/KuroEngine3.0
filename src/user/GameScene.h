#pragma once
#include"KuroEngine.h"
#include"LightManager.h"
#include"Player.h"
#include"GameManager.h"
#include"ShadowMapDevice.h"
#include"LightBloomDevice.h"
#include"NoiseGenerator.h"
#include"IndirectSample.h"

class Model;
class ModelObject;
class GaussianBlur;
class Enemy;
class StaticallyCubeMap;
class DynamicCubeMap;
class TextureBuffer;

class GameScene : public BaseScene
{
	ShadowMapDevice m_shadowMapDevice;
	LightBloomDevice m_lightBloomDevice;

	std::shared_ptr<ModelObject>m_floorModel;

	LightManager m_ligMgr;
	Light::Direction m_dirLig;
	Light::HemiSphere m_hemiLig;
	Light::Point m_ptLig;
	
	Player m_player;

	std::shared_ptr<StaticallyCubeMap>m_staticCubeMap;
	std::shared_ptr<DynamicCubeMap>m_dynamicCubeMap;

	/*struct Noise
	{
		std::shared_ptr<TextureBuffer>tex;
		NoiseInitializer initializer;

		void ResetNoise();
	}noise;*/

	IndirectSample m_indirectSample;

public:
	GameScene();
	void OnInitialize()override;
	void OnUpdate()override;
	void OnDraw()override;
	void OnImguiDebug()override;
	void OnFinalize()override;
};