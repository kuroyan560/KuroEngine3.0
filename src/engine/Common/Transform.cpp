#include "Transform.h"

std::list<Transform*> Transform::s_transformList;

const Matrix& Transform::GetMat(const Matrix& BillBoardMat)
{
	if (!m_dirty)
	{
		bool parentDirty = (m_parent != nullptr && m_parent->m_dirty);
		if (!parentDirty)return m_mat;
	}

	//•Ï‰»‚ ‚èA–¢ŒvZ
	m_mat = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z) * m_rotate;
	m_mat *= BillBoardMat;
	m_mat *= XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

	if (m_parent != nullptr)
	{
		m_mat *= (m_parent->GetMat());
	}

	return m_mat;
}
