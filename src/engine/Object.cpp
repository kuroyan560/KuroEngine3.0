#include "Object.h"
#include"Importer.h"
#include"ModelAnimator.h"
#include"Model.h"

void ModelObject::AttachModel(const std::shared_ptr<Model>& Model)
{
	model = Model;
	
	//アニメーション情報を持つならアニメーター生成
	if (!model->skelton->animations.empty())
	{
		animator = std::make_shared<ModelAnimator>(model);
	}
}

ModelObject::ModelObject(const std::string& Dir, const std::string& FileName)
{
	AttachModel(Importer::Instance()->LoadModel(Dir, FileName));
}

const std::shared_ptr<ConstantBuffer>& ModelObject::GetTransformBuff()
{
	if (!transformBuff)
	{
		transformBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, (model->header.fileName + " - ModelObject").c_str());
	}

	transformBuff->Mapping(&transform.GetMat());

	return transformBuff;
}
