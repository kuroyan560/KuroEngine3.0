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
#include"EnemyManager.h"
#include"Collider.h"

GameScene::GameScene()
{
	floorModel = std::make_shared<ModelObject>("resource/user/", "floor.glb");
	floorModel->transform.SetPos({ 0,-1,0 });
	floorModel->transform.SetScale({ 20.0f,0.0f,20.0f });
	floorModel->model->meshes[0].material->texBuff[COLOR_TEX] = D3D12App::Instance()->GenerateTextureBuffer("resource/user/floor.png");

	shadowMapDevice.SetHeight(100.0f);
	shadowMapDevice.SetBlurPower(4.0f);

	//dirLig.SetDir(Vec3<Angle>(50, -30, 0));
	dirLigTop.SetDir(Vec3<float>(0, 0, -1));
	dirLigFront.SetDir(Vec3<float>(0, 0, 1));
	ligMgr.RegisterDirLight(&dirLigTop);
	ligMgr.RegisterDirLight(&dirLigFront);
	ligMgr.RegisterPointLight(&ptLig);
	hemiLig.SetSkyColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	hemiLig.SetGroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	ligMgr.RegisterHemiSphereLight(&hemiLig);

	GameManager::Instance()->RegisterCamera(Player::CAMERA_KEY, Player::GetCam());

	Transform initSandBagPos;
	
	const float offset = 4.0f;
	for (int x = 0; x < 10; ++x)
	{
		for (int z = 0; z < 10; ++z)
		{
			initSandBagPos.SetPos({ (float)x * offset,2,(float)z * offset });
			EnemyManager::Instance()->Spawn(EnemyManager::SANDBAG, initSandBagPos);
		}
	}
	
	//EnemyManager::Instance()->Spawn(EnemyManager::SANDBAG, initSandBagPos);
}

void GameScene::OnInitialize()
{
	player.Init();
	GameManager::Instance()->ChangeCamera(Player::CAMERA_KEY);
}

void GameScene::OnUpdate()
{
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

	GameManager::Instance()->Update();

	player.Update();

	EnemyManager::Instance()->Update();

	Collider::UpdateAllColliders();
}


void GameScene::OnDraw()
{
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());

	dsv->Clear(D3D12App::Instance()->GetCmdList());

	shadowMapDevice.DrawShadowMap({ player.GetModelObj() });

	//動的キューブマップに書き込み
	//標準描画
	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);

	auto& nowCam = *GameManager::Instance()->GetNowCamera();
	//DrawFunc3D::DrawADSShadingModel(ligMgr, floorModel, nowCam);
	shadowMapDevice.DrawShadowReceiver({ floorModel }, nowCam);

	EnemyManager::Instance()->Draw(nowCam);
	//player.Draw(nowCam);
	DrawFunc3D::DrawPBRShadingModel(ligMgr, player.GetModelObj(), nowCam);

	static bool COLLIDER_DEBUG = false;
	if (COLLIDER_DEBUG)
	{
		Collider::DebugDrawAllColliders(nowCam);
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_RETURN))COLLIDER_DEBUG = !COLLIDER_DEBUG;
}

void GameScene::OnImguiDebug()
{
	//ImguiApp::Instance()->DebugMaterial(sphere->model->meshes[0].material, REWRITE);
	GameManager::Instance()->ImGuiDebug();
}

void GameScene::OnFinalize()
{
}
