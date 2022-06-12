#include "GameScene.h"
#include"KuroEngine.h"
#include"Importer.h"
#include"DrawFunc3D.h"
#include"DrawFunc2D.h"
#include"Camera.h"
#include"Model.h"
#include"Object.h"
#include"GaussianBlur.h"
#include"CubeMap.h"
#include"ModelAnimator.h"

GameScene::GameScene()
{
	animModel = std::make_shared<ModelObject>("resource/user/player_anim_test/", "player_anim_test.glb");

	//dirLig.SetDir(Vec3<Angle>(50, -30, 0));
	dirLigTop.SetDir(Vec3<float>(0, 0, -1));
	dirLigFront.SetDir(Vec3<float>(0, 0, 1));
	ligMgr.RegisterDirLight(&dirLigTop);
	ligMgr.RegisterDirLight(&dirLigFront);
	ligMgr.RegisterPointLight(&ptLig);
	hemiLig.SetSkyColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	hemiLig.SetGroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	ligMgr.RegisterHemiSphereLight(&hemiLig);
}

void GameScene::OnInitialize()
{
	debugCam.Init({ 0,1,-3 }, { 0,1,0 });
}

void GameScene::OnUpdate()
{
	if (UsersInput::Instance()->KeyOnTrigger(DIK_I))
	{
		debugCam.Init({ 0,1,-3 }, { 0,1,0 });
		animModel->animator->Reset();
	}

	//ポイントライト位置
	static const float UINT = 0.1f;

	//テストモデルの位置
	//auto modelPos = testModel->transform.GetPos();
	auto modelPos = ptLig.GetPos();
	if (UsersInput::Instance()->KeyInput(DIK_E))
	{
		modelPos.y += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_Q))
	{
		modelPos.y -= UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_D))
	{
		modelPos.x += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_A))
	{
		modelPos.x -= UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_W))
	{
		modelPos.z += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_S))
	{
		modelPos.z -= UINT;
	}

	//testModel->transform.SetPos(modelPos);
	ptLig.SetPos(modelPos);

	//ライトのON/OFF
	if (UsersInput::Instance()->KeyOnTrigger(DIK_1))
	{
		dirLigFront.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_2))
	{
		dirLigTop.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_3))
	{
		hemiLig.SetActive();
	}

	//アニメーション
	if (UsersInput::Instance()->KeyOnTrigger(DIK_L))
	{
		animModel->animator->Play("Bones|Action1", false);
	}
	animModel->animator->Update();

	debugCam.Move();
}


void GameScene::OnDraw()
{
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());

	dsv->Clear(D3D12App::Instance()->GetCmdList());

	//動的キューブマップに書き込み
	//標準描画
	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);

	DrawFunc3D::DrawADSShadingModel(ligMgr, animModel, debugCam);
}

void GameScene::OnImguiDebug()
{
	//ImguiApp::Instance()->DebugMaterial(sphere->model->meshes[0].material, REWRITE);
}

void GameScene::OnFinalize()
{
}
