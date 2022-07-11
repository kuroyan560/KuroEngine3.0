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

	noise.ResetNoise();
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
	//staticCubeMap->Draw(nowCam);

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

	if (UsersInput::Instance()->KeyOnTrigger(DIK_SPACE))
	{
		noise.ResetNoise();
	}
	//DrawFunc2D::DrawGraph({ 0,0 }, noise.tex, AlphaBlendMode_None);
	HitEffect::Draw(nowCam);
}

void GameScene::OnImguiDebug()
{
	/*
	ImGui::Begin("Noise");
	bool change = false;

	static int TYPE_IDX = WAVELET;
	static std::string CURRENT_TYPE = NoiseGenerator::GetInterpolationName((NOISE_INTERPOLATION)TYPE_IDX);

	if (ImGui::BeginCombo("Interpolation", CURRENT_TYPE.c_str()))
	{
		for (int n = 0; n < NOISE_INTERPOLATION_NUM; ++n)
		{
			auto interpolationName = NoiseGenerator::GetInterpolationName((NOISE_INTERPOLATION)n);
			bool isSelected = (CURRENT_TYPE == interpolationName);
			if (ImGui::Selectable(interpolationName.c_str(), isSelected))
			{
				CURRENT_TYPE = interpolationName;
				TYPE_IDX = n;
				noise.initializer.interpolation = (NOISE_INTERPOLATION)n;
				change = true;
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	if (ImGui::SliderInt("Split - X", &noise.initializer.split.x, 1, 64))change = true;
	if (ImGui::SliderInt("Split - Y", &noise.initializer.split.y, 1, 64))change = true;
	if (ImGui::SliderInt("Contrast", &noise.initializer.contrast, 1, 32))change = true;
	if (ImGui::SliderInt("Octaves", &noise.initializer.octave, 1, 32))change = true;
	if (ImGui::SliderFloat("Frequency", &noise.initializer.frequency, 1.0f, 32.0f))change = true;
	if (ImGui::SliderFloat("Persistance", &noise.initializer.persistance, 0.0f, 1.0f))change = true;
	if (change)
	{
		noise.ResetNoise();
	}
	ImGui::End();
	*/

	GameManager::Instance()->ImGuiDebug();
}

void GameScene::OnFinalize()
{
}

void GameScene::Noise::ResetNoise()
{
	if (!tex)
	{
		tex = NoiseGenerator::PerlinNoise2D("DebugNoise", { 512,512 }, initializer, DXGI_FORMAT_R32G32B32A32_FLOAT);
	}
	else
	{
		NoiseGenerator::PerlinNoise2D(tex, initializer);
	}
}
