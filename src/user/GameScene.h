#pragma once
#include"KuroEngine.h"
#include"LightManager.h"
#include"Player.h"
#include"GameManager.h"
#include"ShadowMapDevice.h"
#include"LightBloomDevice.h"

class Model;
class ModelObject;
class GaussianBlur;
class Enemy;
class StaticallyCubeMap;
class DynamicCubeMap;

class GameScene : public BaseScene
{
	ShadowMapDevice shadowMapDevice;
	LightBloomDevice lightBloomDevice;

	std::shared_ptr<ModelObject>floorModel;

	LightManager ligMgr;
	Light::Direction dirLig;
	Light::HemiSphere hemiLig;
	Light::Point ptLig;
	
	Player player;

	std::shared_ptr<StaticallyCubeMap>staticCubeMap;
	std::shared_ptr<DynamicCubeMap>dynamicCubeMap;

public:
	GameScene();
	void OnInitialize()override;
	void OnUpdate()override;
	void OnDraw()override;
	void OnImguiDebug()override;
	void OnFinalize()override;
};