#include "Transform2D.h"

std::list<Transform2D*> Transform2D::s_transform2DList;

const Matrix& Transform2D::GetMat()
{
	if (!m_dirty)
	{
		bool parentDirty = (m_parent != nullptr && m_parent->m_dirty);
		if (!parentDirty)return m_mat;
	}

	m_mat = XMMatrixScaling(m_scale.x, m_scale.y, 1.0f) * m_rotate;
	m_mat.r[3].m128_f32[0] = m_pos.x;
	m_mat.r[3].m128_f32[1] = m_pos.y;

	if (m_parent != nullptr)
	{
		m_mat *= m_parent->GetMat();
	}

	return m_mat;
}
