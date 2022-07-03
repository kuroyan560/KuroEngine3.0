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
#include"NoiseGenerator.h"
#include"HitEffect.h"
#include"CubeMap.h"

GameScene::GameScene()
{
	floorModel = std::make_shared<ModelObject>("resource/user/", "floor.glb");
	floorModel->transform.SetPos({ 0,-1,0 });
	floorModel->transform.SetScale({ 20.0f,0.0f,20.0f });
	floorModel->model->meshes[0].material->texBuff[COLOR_TEX] = D3D12App::Instance()->GenerateTextureBuffer("resource/user/floor.png");

	shadowMapDevice.SetHeight(100.0f);
	shadowMapDevice.SetBlurPower(4.0f);

	dirLig.SetDir(Vec3<float>(0, 0, -1));
	ligMgr.RegisterDirLight(&dirLig);
	ligMgr.RegisterPointLight(&ptLig);
	hemiLig.SetSkyColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	hemiLig.SetGroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	ligMgr.RegisterHemiSphereLight(&hemiLig);

	GameManager::Instance()->RegisterCamera(Player::CAMERA_KEY, Player::GetCam());

	Transform sandbagTrans;

	//const float offset = 4.0f;
	//for (int x = 0; x < 10; ++x)
	//{
	//	for (int z = 0; z < 10; ++z)
	//	{
	//		initSandBagPos.SetPos({ (float)x * offset,2,(float)z * offset });
	//		EnemyManager::Instance()->Spawn(EnemyManager::SANDBAG, initSandBagPos);
	//	}
	//}

	//sandbagTrans.SetScale(7);
	sandbagTrans.SetPos({ 0,6,0 });
	EnemyManager::Instance()->Spawn(SANDBAG, sandbagTrans);

	staticCubeMap = std::make_shared<StaticallyCubeMap>("SkyBox");
	const std::string cubeMpaDir = "resource/user/hdri/";
	staticCubeMap->AttachCubeMapTex(D3D12App::Instance()->GenerateTextureBuffer(cubeMpaDir + "hdri_cube.dds", true));
	staticCubeMap->AttachTex(cubeMpaDir, ".png");
	staticCubeMap->transform.SetScale(30);

	dynamicCubeMap = std::make_shared<DynamicCubeMap>();
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

	if (UsersInput::Instance()->KeyOnTrigger(DIK_2))
	{
		dirLig.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_3))
	{
		hemiLig.SetActive();
	}

	if (UsersInput::Instance()->ControllerOnTrigger(0,XBOX_BUTTON::A))
	{
		HitEffect::Generate({ 0.0f,7.0f,0.0f });
	}

	GameManager::Instance()->Update();

	player.Update();

	EnemyManager::Instance()->Update();

	Collider::UpdateAllColliders();

	HitEffect::Update();
}


void GameScene::OnDraw()
{
	//キューブマップに描き込む
	dynamicCubeMap->Clear();
	dynamicCubeMap->CopyCubeMap(staticCubeMap);
	dynamicCubeMap->DrawToCubeMap(ligMgr, { player.GetModelObj() });

	//デプスステンシル
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());
	dsv->Clear(D3D12App::Instance()->GetCmdList());

	//シャドウマップに描き込み
	shadowMapDevice.DrawShadowMap({ player.GetModelObj() });

	//標準描画
	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);

	//現在のカメラ取得
	auto& nowCam = *GameManager::Instance()->GetNowCamera();

	//キューブマップ描画
	staticCubeMap->Draw(nowCam);

	//影つき床
	shadowMapDevice.DrawShadowReceiver({ floorModel }, nowCam);

	//敵
	EnemyManager::Instance()->Draw(nowCam, dynamicCubeMap);

	//プレイヤー
	DrawFunc3D::DrawPBRShadingModel(ligMgr, player.GetModelObj(), nowCam, staticCubeMap);

	static Transform debugTrans;
	debugTrans.SetPos({ 9,6,0 });
	//DrawFunc3D::DrawPBRShadingModel(ligMgr, EnemyManager::Instance()->GetModel(SANDBAG), debugTrans, nowCam, nullptr, cubeMap);

	//当たり判定デバッグ描画
	static bool COLLIDER_DRAW = true;
	if (UsersInput::Instance()->ControllerOnTrigger(0, XBOX_BUTTON::X))
	{
		COLLIDER_DRAW = !COLLIDER_DRAW;
	}
	if (COLLIDER_DRAW)
	{
		Collider::DebugDrawAllColliders(nowCam);
	}

	//ヒットエフェクト
	//DrawFunc2D::DrawBox2D({ 0,0 }, WinApp::Instance()->GetExpandWinSize(), Color(0, 0, 0, 1), true, AlphaBlendMode_None);
	HitEffect::Draw(nowCam);
}

void GameScene::OnImguiDebug()
{
	ImGui::Begin("Button");
	ImGui::Text("RB - Player's attack");
	ImGui::Text("A  - Emit hit effect");
	ImGui::Text("X - Turn collider's draw flag");

	ImGui::End();

	GameManager::Instance()->ImGuiDebug();
}

void GameScene::OnFinalize()
{
}
