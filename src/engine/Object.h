#pragma once
#include<memory>
#include"Transform.h"
class Model;
#include<string>
class ConstantBuffer;

struct ModelObject
{
private:
	std::shared_ptr<ConstantBuffer>transformBuff;

public:
	std::shared_ptr<Model>model;
	Transform transform;

	ModelObject() {}
	ModelObject(const std::string& Dir, const std::string& FileName);
	ModelObject(const std::shared_ptr<Model>& Model) :model(Model) {}

	const std::shared_ptr<ConstantBuffer>&GetTransformBuff();
};