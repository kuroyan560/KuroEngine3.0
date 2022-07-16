#include "Model.h"

Vec3<ValueMinMax> Model::GetAllMeshPosMinMax()
{
	std::vector<Vec3<ValueMinMax>>values;

	for (const auto& mesh : m_meshes)
	{
		values.emplace_back(mesh.GetPosMinMax());
	}

	Vec3<ValueMinMax>result;
	result.x.Set();
	result.y.Set();
	result.z.Set();

	for (const auto& val : values)
	{
		if (val.x.m_min < result.x.m_min)result.x.m_min = val.x.m_min;
		if (result.x.m_max < val.x.m_max)result.x.m_max = val.x.m_max;
		if (val.y.m_min < result.y.m_min)result.y.m_min = val.y.m_min;
		if (result.y.m_max < val.y.m_max)result.y.m_max = val.y.m_max;
		if (val.z.m_min < result.z.m_min)result.z.m_min = val.z.m_min;
		if (result.z.m_max < val.z.m_max)result.z.m_max = val.z.m_max;
	}

	return result;
}
