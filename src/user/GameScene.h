#pragma once
#include"KuroEngine.h"
#include"DebugCamera.h"
#include"LightManager.h"
#include"Transform.h"
#include<array>
#include"ShadowMapDevice.h"
#include"CubeMap.h"

class Model;
class ModelObject;
class GaussianBlur;

class GameScene : public BaseScene
{
	std::array<std::shared_ptr<ModelObject>, 2>animModel;
	int nowModel = 0;
	std::array<std::shared_ptr<ModelObject>, 2>axisModels;
	int nowAxis = 0;

	DebugCamera debugCam;
	LightManager ligMgr;
	Light::Direction dirLigTop;
	Light::Direction dirLigFront;
	Light::HemiSphere hemiLig;
	Light::Point ptLig;
	Transform trans;

public:
	GameScene();
	void OnInitialize()override;
	void OnUpdate()override;
	void OnDraw()override;
	void OnImguiDebug()override;
	void OnFinalize()override;
};