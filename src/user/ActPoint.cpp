#include "ActPoint.h"
#include"DrawFuncBillBoard.h"
#include"DrawFunc3D.h"

int ActPoint::s_id = 0;
std::vector<ActPoint*>ActPoint::s_points;

void ActPoint::DebugDraw(Camera& Cam)
{
	static Vec2<float>s_size = { 1.0,1.0 };
	for (auto& p : s_points)
	{
		Color col;
		if (!p->m_isActive)
		{
			col = Color(0.4f, 0.4f, 0.4f, 1.0f);
		}
		else if (p->m_canRockOn)
		{
			if (p->m_canMarking)
			{
				col = Color(0.0f, 0.0f, 1.0f, 1.0f);
			}
			else
			{
				col = Color(1.0f, 0.0f, 0.0f, 1.0f);
			}
		}
		else if (p->m_canMarking)
		{
			col = Color(0.0f, 1.0f, 0.0f, 1.0f);
		}
		DrawFuncBillBoard::Box(Cam, p->m_pos, s_size, col);
	}
}
