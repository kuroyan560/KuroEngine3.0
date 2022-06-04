#include "Object.h"
#include"Importer.h"
#include"Model.h"

ModelObject::ModelObject(const std::string& Dir, const std::string& FileName)
{
	model = Importer::Instance()->LoadModel(Dir, FileName);
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
