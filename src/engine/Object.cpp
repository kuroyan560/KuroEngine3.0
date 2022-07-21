#include "Object.h"
#include"Importer.h"
#include"ModelAnimator.h"
#include"Model.h"

void ModelObject::AttachModel(const std::shared_ptr<Model>& Model)
{
	m_model = Model;
	
	//アニメーション情報を持つならアニメーター生成
	if (!m_model->m_skelton->animations.empty())
	{
		m_animator = std::make_shared<ModelAnimator>(m_model);
	}
}

ModelObject::ModelObject(const std::string& Dir, const std::string& FileName)
{
	AttachModel(Importer::Instance()->LoadModel(Dir, FileName));
}

const std::shared_ptr<ConstantBuffer>& ModelObject::GetTransformBuff()
{
	if (!m_transformBuff)
	{
		m_transformBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, (m_model->m_header.fileName + " - ModelObject - Transform").c_str());
	}

	m_transformBuff->Mapping(&m_transform.GetMat());

	return m_transformBuff;
}