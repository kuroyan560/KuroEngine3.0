#pragma once
#include"KuroEngine.h"
#include"DebugCamera.h"
#include"LightManager.h"
#include"Transform.h"
#include<array>
#include"ShadowMapDevice.h"

class Model;
class ModelObject;
class GaussianBlur;
class CubeMap;

class GameScene : public BaseScene
{
	std::shared_ptr<ModelObject>testModel;

	DebugCamera debugCam;
	LightManager ligMgr;
	Light::Direction dirLigTop;
	Light::Direction dirLigFront;
	Light::HemiSphere hemiLig;
	Transform trans;

	std::shared_ptr<CubeMap>yokohamaCubeMap;
	std::shared_ptr<CubeMap>skyCubeMap;

public:
	GameScene();
	void OnInitialize()override;
	void OnUpdate()override;
	void OnDraw()override;
	void OnImguiDebug()override;
	void OnFinalize()override;
};