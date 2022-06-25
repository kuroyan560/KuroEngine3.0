#pragma once
#include"KuroEngine.h"
#include"LightManager.h"
#include"Player.h"
#include"GameManager.h"
#include"ShadowMapDevice.h"

class Model;
class ModelObject;
class GaussianBlur;
class Enemy;

class GameScene : public BaseScene
{
	ShadowMapDevice shadowMapDevice;
	std::shared_ptr<ModelObject>floorModel;

	LightManager ligMgr;
	Light::Direction dirLigTop;
	Light::Direction dirLigFront;
	Light::HemiSphere hemiLig;
	Light::Point ptLig;
	
	Player player;

public:
	GameScene();
	void OnInitialize()override;
	void OnUpdate()override;
	void OnDraw()override;
	void OnImguiDebug()override;
	void OnFinalize()override;
};