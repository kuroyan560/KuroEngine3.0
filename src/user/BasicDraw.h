#pragma once
#include<memory>
#include"LightBloomDevice.h"
class LightManager;
class ModelObject;
class Camera;
class CubeMap;
class BasicDraw
{
	static int s_drawCount;
public:
	static void CountReset() { s_drawCount = 0; }
	static void Draw(LightManager& LigManager, const std::weak_ptr<ModelObject>ModelObject, Camera& Cam, std::shared_ptr<CubeMap>AttachCubeMap = nullptr);
};