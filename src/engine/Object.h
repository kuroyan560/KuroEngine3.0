#pragma once
#include<memory>
class ConstantBuffer;
#include"Transform.h"
#include<string>
class ModelAnimator;
class Model;

class ModelObject
{
private:
	std::shared_ptr<ConstantBuffer>m_transformBuff;
	std::shared_ptr<ConstantBuffer>m_boneBuff;
	void AttachModel(const std::shared_ptr<Model>& Model);

public:
	std::shared_ptr<Model>m_model;
	std::shared_ptr<ModelAnimator>m_animator;
	Transform m_transform;

	ModelObject(const std::string& Dir, const std::string& FileName);
	ModelObject(const std::shared_ptr<Model>& Model) { AttachModel(Model); }

	const std::shared_ptr<ConstantBuffer>&GetTransformBuff();
};