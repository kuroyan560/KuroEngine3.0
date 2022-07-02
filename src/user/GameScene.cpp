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

void GameScene::NoiseGenerate()
{
	if (!noises[0].noise)
	{
		noises[0].noise = NoiseGenerator::PerlinNoise(noiseSize, noises[0].split, noises[0].octaves, noises[0].frequency, noises[0].persistance);
		noises[1].noise = NoiseGenerator::PerlinNoise(noiseSize, noises[1].split, noises[1].octaves, noises[1].frequency, noises[1].persistance);
	}
	else
	{
		NoiseGenerator::PerlinNoise(noises[0].noise, noises[0].split, noises[0].octaves, noises[0].frequency, noises[0].persistance);
		NoiseGenerator::PerlinNoise(noises[1].noise, noises[1].split, noises[1].octaves, noises[1].frequency, noises[1].persistance);
	}
}

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
	noises[1].split = 7;
	noises[1].octaves = 2;
	noises[1].frequency = 0.79f;
	noises[1].persistance = 0.5f;

	noises[0].split = 12;
	noises[0].octaves = 6;
	noises[0].frequency = 1.647f;
	noises[0].persistance = 0.775f;
	NoiseGenerate();
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
		HitEffect::Generate(WinApp::Instance()->GetExpandWinCenter());
	}

	GameManager::Instance()->Update();

	player.Update();

	EnemyManager::Instance()->Update();

	Collider::UpdateAllColliders();

	HitEffect::Update();
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

	Collider::DebugDrawAllColliders(nowCam);

	if (UsersInput::Instance()->KeyInput(DIK_SPACE))
	{
		NoiseGenerate();
	}
	DrawFunc2D::DrawGraph({ 0,0 }, noises[0].noise);
	//DrawFunc2D::DrawGraph({ 530,0 }, noise2);

	//DrawFunc2D::DrawBox2D({ 0,0 }, WinApp::Instance()->GetExpandWinSize(), Color(0, 0, 0, 1), true, AlphaBlendMode_None);
	HitEffect::Draw(noises[0].noise,noises[1].noise);

	lightBloomDevice.SetRenderTargets();
	HitEffect::Draw(noises[0].noise,noises[1].noise);

	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);
	lightBloomDevice.Draw();
}

void GameScene::OnImguiDebug()
{
	//ImguiApp::Instance()->DebugMaterial(sphere->model->meshes[0].material, REWRITE);

	for (int i = 0; i < 2; ++i)
	{
		ImGui::Begin(("Noise" + std::to_string(i)).c_str());
		bool change = false;
		if (ImGui::SliderInt("Split", &noises[i].split, 1, 256))change = true;
		if (ImGui::SliderInt("Octaves", &noises[i].octaves, 1, 10))change = true;
		if (ImGui::SliderFloat("Frequency", &noises[i].frequency, 0.1f, 10.0f))change = true;
		if (ImGui::SliderFloat("Persistance", &noises[i].persistance, 0.1f, 1.0f))change = true;

		if (change)
		{
			NoiseGenerate();
		}

		ImGui::End();
	}

	ImGui::Begin("Effect");
	ImGui::SliderFloat("Blur", &HitEffect::GetInstance(0).blur, 0.0f, 50.0f);
	ImGui::SliderFloat("Scale", &HitEffect::GetInstance(0).scale, 0.0f, 5.0f);
	ImGui::SliderFloat("ToOutUVOffset", &HitEffect::GetInstance(0).uvRadiusOffset, -1.0f, 1.0f);
	ImGui::SliderFloat("CircleThickness", &HitEffect::GetInstance(0).circleThickness, 0.0f, 1.0f);
	ImGui::SliderFloat("CircleRadius", &HitEffect::GetInstance(0).circleRadius, 0.0f, 1.0f);
	ImGui::End();
	//GameManager::Instance()->ImGuiDebug();
}

void GameScene::OnFinalize()
{
}
