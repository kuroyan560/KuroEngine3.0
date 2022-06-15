#include "Transform.h"

std::list<Transform*> Transform::TRANSFORMS;

const Matrix& Transform::GetMat(const Matrix& BillBoardMat)
{
	if (!dirty)
	{
		bool parentDirty = (parent != nullptr && parent->dirty);
		if (!parentDirty)return mat;
	}

	//•Ï‰»‚ ‚èA–¢ŒvZ
	mat = XMMatrixScaling(scale.x, scale.y, scale.z) * rotate;
	mat *= BillBoardMat;
	mat *= XMMatrixTranslation(pos.x, pos.y, pos.z);

	if (parent != nullptr)
	{
		mat *= (parent->GetMat());
	}

	return mat;
}
