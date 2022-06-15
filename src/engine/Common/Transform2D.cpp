#include "Transform2D.h"

std::list<Transform2D*> Transform2D::TRANSFORMS;

const Matrix& Transform2D::GetMat()
{
	if (!dirty)
	{
		bool parentDirty = (parent != nullptr && parent->dirty);
		if (!parentDirty)return mat;
	}

	mat = XMMatrixScaling(scale.x, scale.y, 1.0f) * rotate;
	mat.r[3].m128_f32[0] = pos.x;
	mat.r[3].m128_f32[1] = pos.y;

	if (parent != nullptr)
	{
		mat *= parent->GetMat();
	}

	return mat;
}
