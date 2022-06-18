#include "Model.h"

Vec3<ValueMinMax> Model::GetAllMeshPosMinMax()
{
	std::vector<Vec3<ValueMinMax>>values;

	for (const auto& mesh : meshes)
	{
		values.emplace_back(mesh.GetPosMinMax());
	}

	Vec3<ValueMinMax>result;
	result.x.Set();
	result.y.Set();
	result.z.Set();

	for (const auto& val : values)
	{
		if (val.x.min < result.x.min)result.x.min = val.x.min;
		if (result.x.max < val.x.max)result.x.max = val.x.max;
		if (val.y.min < result.y.min)result.y.min = val.y.min;
		if (result.y.max < val.y.max)result.y.max = val.y.max;
		if (val.z.min < result.z.min)result.z.min = val.z.min;
		if (result.z.max < val.z.max)result.z.max = val.z.max;
	}

	return result;
}
