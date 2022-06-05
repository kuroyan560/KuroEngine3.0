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
	enum MODEL_NAME{ WOOD_CUBE, STONE_BALL, METAL_BALL, MODEL_NUM }nowModel = WOOD_CUBE;
	std::array<std::shared_ptr<ModelObject>, MODEL_NUM>models;
	std::shared_ptr<ModelObject>testModel;

	DebugCamera debugCam;
	LightManager ligMgr;
	Light::Direction dirLigTop;
	Light::Direction dirLigFront;
	Light::HemiSphere hemiLig;
	Light::Point ptLig;
	Transform trans;

	std::shared_ptr<StaticallyCubeMap>yokohamaCubeMap;
	std::shared_ptr<StaticallyCubeMap>skyCubeMap;
	std::shared_ptr<StaticallyCubeMap>hdriCubeMap;
	std::shared_ptr<DynamicCubeMap>dynamicCubeMap;

public:
	GameScene();
	void OnInitialize()override;
	void OnUpdate()override;
	void OnDraw()override;
	void OnImguiDebug()override;
	void OnFinalize()override;
};