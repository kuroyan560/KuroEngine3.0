#pragma once
#include<memory>
class ConstantBuffer;
#include"Transform.h"
#include<string>
#include"Model.h"
//#include"ModelAnimator.h"

struct ModelObject
{
private:
	std::shared_ptr<ConstantBuffer>transformBuff;
	void AttachModel(const std::shared_ptr<Model>& Model);

public:
	std::shared_ptr<Model>model;
	//std::shared_ptr<ModelAnimator>animator;
	Transform transform;

	ModelObject(const std::string& Dir, const std::string& FileName);
	ModelObject(const std::shared_ptr<Model>& Model) { AttachModel(Model); }

	const std::shared_ptr<ConstantBuffer>&GetTransformBuff();
};