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
	m_floorModel = std::make_shared<ModelObject>("resource/user/", "floor.glb");
	m_floorModel->m_transform.SetPos({ 0,-1,0 });
	m_floorModel->m_transform.SetScale({ 20.0f,0.0f,20.0f });
	m_floorModel->m_model->m_meshes[0].material->texBuff[COLOR_TEX] = D3D12App::Instance()->GenerateTextureBuffer("resource/user/floor.png");

	m_shadowMapDevice.SetHeight(100.0f);
	m_shadowMapDevice.SetBlurPower(4.0f);

	m_dirLig.SetDir(Vec3<float>(0, 0, -1));
	m_ligMgr.RegisterDirLight(&m_dirLig);
	m_ligMgr.RegisterPointLight(&m_ptLig);
	m_hemiLig.SetSkyColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	m_hemiLig.SetGroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	m_ligMgr.RegisterHemiSphereLight(&m_hemiLig);

	GameManager::Instance()->RegisterCamera(Player::s_cameraKey, Player::GetCam());

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

	m_staticCubeMap = std::make_shared<StaticallyCubeMap>("SkyBox");
	const std::string cubeMpaDir = "resource/user/hdri/";
	m_staticCubeMap->AttachCubeMapTex(D3D12App::Instance()->GenerateTextureBuffer(cubeMpaDir + "hdri_cube.dds", true));
	m_staticCubeMap->AttachTex(cubeMpaDir, ".png");
	m_staticCubeMap->m_transform.SetScale(30);

	m_dynamicCubeMap = std::make_shared<DynamicCubeMap>();

	//noise.ResetNoise();
}

void GameScene::OnInitialize()
{
	m_player.Init();
	//GameManager::Instance()->ChangeCamera(Player::CAMERA_KEY);
	m_indirectSample.Init(*GameManager::Instance()->GetNowCamera());
}

void GameScene::OnUpdate()
{
	//ポイントライト位置
	static const float UINT = 0.1f;

	//テストモデルの位置
	//auto modelPos = testModel->transform.GetPos();
	auto modelPos = m_ptLig.GetPos();
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
	m_ptLig.SetPos(modelPos);

	if (UsersInput::Instance()->KeyOnTrigger(DIK_2))
	{
		m_dirLig.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_3))
	{
		m_hemiLig.SetActive();
	}

	if (UsersInput::Instance()->ControllerOnTrigger(0,XBOX_BUTTON::A))
	{
		HitEffect::Generate({ 0.0f,7.0f,0.0f });
	}

	GameManager::Instance()->Update();

	m_player.Update();

	EnemyManager::Instance()->Update();

	Collider::UpdateAllColliders();

	HitEffect::Update();

	m_indirectSample.Update(m_enableCalling);
	//m_indirectSample.Update(m_enableCalling, *GameManager::Instance()->GetNowCamera());
}


void GameScene::OnDraw()
{
	auto cmdList = D3D12App::Instance()->GetCmdList();

	//デプスステンシル
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());
	dsv->Clear(cmdList);

	//現在のカメラ取得
	auto& nowCam = *GameManager::Instance()->GetNowCamera();
	nowCam.GetBuff();

	//GraphicsManagerの管轄外
	{
		auto backRT = D3D12App::Instance()->GetBackBuffRenderTarget();

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
		rtvs.emplace_back(backRT->AsRTV(cmdList));

		const Vec2<float> targetSize = backRT->GetGraphSize().Float();
		//ビューポート設定
		auto viewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, targetSize.x, targetSize.y);
		cmdList->RSSetViewports(1, &viewPort);

		//シザー矩形設定
		auto rect = CD3DX12_RECT(0, 0, static_cast<LONG>(targetSize.x), static_cast<LONG>(targetSize.y));
		cmdList->RSSetScissorRects(1, &rect);

		cmdList->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), &rtvs[0], FALSE, dsv->AsDSV(cmdList));

		m_indirectSample.Draw(nowCam);
	}

	/*
	//キューブマップに描き込む
	dynamicCubeMap->Clear();
	dynamicCubeMap->CopyCubeMap(staticCubeMap);
	dynamicCubeMap->DrawToCubeMap(ligMgr, { player.GetModelObj() });

	//シャドウマップに描き込み
	shadowMapDevice.DrawShadowMap({ player.GetModelObj() });

	//標準描画
	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);



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
	DrawFunc2D::DrawGraph({ 0,0 }, noise.tex, AlphaBlendMode_None);
	HitEffect::Draw(nowCam);
	*/
}

void GameScene::OnImguiDebug()
{
	ImGui::Begin("Indirect");
	ImGui::Checkbox("EnableCalling", &m_enableCalling);
	ImGui::End();

	/*ImGui::Begin("Button");
	ImGui::Text("RB - Player's attack");
	ImGui::Text("A  - Emit hit effect");
	ImGui::Text("X - Turn collider's draw flag");
	ImGui::End();

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

	GameManager::Instance()->ImGuiDebug();
	*/
}

void GameScene::OnFinalize()
{
}

/*
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
*/