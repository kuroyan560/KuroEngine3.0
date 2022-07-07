#include "SpriteMesh.h"
#include"Mesh.h"
#include"KuroEngine.h"

SpriteMesh::SpriteMesh(const char* Name)
{
	//メッシュ生成
	mesh = std::make_shared<Mesh<SpriteMesh::Vertex>>();

	//名前があったら設定
	if(Name)mesh->name = Name;

	//頂点設定
	mesh->vertices.resize(IDX_NUM);
	mesh->vertices[LB].uv = { 0.0f,1.0f };
	mesh->vertices[LT].uv = { 0.0f,0.0f };
	mesh->vertices[RB].uv = { 1.0f,1.0f };
	mesh->vertices[RT].uv = { 1.0f,0.0f };

	//バッファ生成
	mesh->CreateBuff();

	//トリミング初期化
	trim[TOP] = 0.0f;
	trim[BOTTOM] = 1.0f;
	trim[LEFT] = 0.0f;
	trim[RIGHT] = 1.0f;
}

void SpriteMesh::SetUv(const float& Top, const float& Buttom, const float& Left, const float& Right)
{
	mesh->vertices[LT].uv = { Left,Top };
	mesh->vertices[LB].uv = { Left,Buttom };
	mesh->vertices[RT].uv = { Right,Top };
	mesh->vertices[RB].uv = { Right,Buttom };

	dirty = true;
}

void SpriteMesh::Render(const std::vector<std::shared_ptr<DescriptorData>>& DescDatas, const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes, const int& InstanceNum)
{
	if (dirty)
	{
		//ウィンドウサイズ倍率に合わせる
		Vec2<float>resultSize = size * WinApp::Instance()->GetWinDifferRate();

		float left = (0.0f - anchorPoint.x) * resultSize.x;
		float right = (1.0f - anchorPoint.x) * resultSize.x;
		float top = (0.0f - anchorPoint.y) * resultSize.y;
		float bottom = (1.0f - anchorPoint.y) * resultSize.y;

		if (flip.x)	//左右反転
		{
			left *= -1;
			right *= -1;
		}
		if (flip.y)	//上下反転
		{
			top *= -1;
			bottom *= -1;
		}

		mesh->vertices[LB].pos = { left,bottom };		//左下
		mesh->vertices[LT].pos = { left,top };			//左上
		mesh->vertices[RB].pos = { right,bottom };		//右下
		mesh->vertices[RT].pos = { right,top };			//右上

		//トリミング加工
		{
			mesh->vertices[LT].pos.y += trim[TOP] * resultSize.y;
			mesh->vertices[RT].pos.y += trim[TOP] * resultSize.y;
			mesh->vertices[LT].uv.y += trim[TOP];
			mesh->vertices[RT].uv.y += trim[TOP];

			mesh->vertices[LB].pos.y += (1 - trim[BOTTOM]) * -resultSize.y;
			mesh->vertices[RB].pos.y += (1 - trim[BOTTOM]) * -resultSize.y;
			mesh->vertices[LB].uv.y += -(1 - trim[BOTTOM]);
			mesh->vertices[RB].uv.y += -(1 - trim[BOTTOM]);

			mesh->vertices[LT].pos.x += trim[LEFT] * resultSize.x;
			mesh->vertices[LB].pos.x += trim[LEFT] * resultSize.x;
			mesh->vertices[LT].uv.x += trim[LEFT];
			mesh->vertices[LB].uv.x += trim[LEFT];

			mesh->vertices[RT].pos.x += (1 - trim[RIGHT]) * -resultSize.x;
			mesh->vertices[RB].pos.x += (1 - trim[RIGHT]) * -resultSize.x;
			mesh->vertices[RT].uv.x += -(1 - trim[RIGHT]);
			mesh->vertices[RB].uv.x += -(1 - trim[RIGHT]);
		}

		//マッピング
		mesh->Mapping();

		dirty = false;
	}

	KuroEngine::Instance().Graphics().ObjectRender(mesh->vertBuff, DescDatas, DescHandleTypes, zLayer, trans, InstanceNum);
}
