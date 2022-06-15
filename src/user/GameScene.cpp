#include "GameScene.h"
#include"KuroEngine.h"
#include"DrawFunc2D.h"

GameScene::GameScene()
{
}

void GameScene::OnInitialize()
{
}

void GameScene::OnUpdate()
{
}


void GameScene::OnDraw()
{
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());

	dsv->Clear(D3D12App::Instance()->GetCmdList());

	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);

}

void GameScene::OnImguiDebug()
{
	//ImguiApp::Instance()->DebugMaterial(sphere->model->meshes[0].material, REWRITE);
}

void GameScene::OnFinalize()
{
}
