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
#include"BasicDraw.h"

GameScene::GameScene()
{
	//床
	m_floorModel = std::make_shared<ModelObject>("resource/user/", "floor.glb");
	m_floorModel->m_transform.SetPos({ 0,-1,0 });
	m_floorModel->m_transform.SetScale({ 20.0f,0.0f,20.0f });
	m_floorModel->m_model->m_meshes[0].material->texBuff[COLOR_TEX] = D3D12App::Instance()->GenerateTextureBuffer("resource/user/floor.png");

	//床用コライダー
	auto floorCol_Mesh = std::make_shared<CollisionMesh>(
		m_floorModel->m_model->m_meshes[0].GetCollisionTriangles(),
		&m_floorModel->m_transform, nullptr, 
		true);
	m_floorCol = Collider::Generate(floorCol_Mesh);
	m_floorCol->SetMyAttribute(FLOOR);
	m_floorCol->SetHitCheckAttribute(FOOT_POINT);

	//テスト用
	//m_testObj = std::make_shared<ModelObject>("resource/user/", "player_anim_test.glb");
	//m_testObj->m_transform.SetScale(8.0f);

	m_dirLig.SetDir(Vec3<float>(0, -1, 0));
	m_hemiLig.SetSkyColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	m_hemiLig.SetGroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	m_ligMgr.RegisterDirLight(&m_dirLig);
	//m_ligMgr.RegisterPointLight(&m_ptLig);
	//m_ligMgr.RegisterHemiSphereLight(&m_hemiLig);

	GameManager::Instance()->RegisterCamera(Player::s_cameraKey, Player::GetCam());

	Transform sandbagTrans;
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
	GameManager::Instance()->ChangeCamera(Player::s_cameraKey);
	HitEffect::Init(*GameManager::Instance()->GetNowCamera());
}

void GameScene::OnUpdate()
{
	GameManager::Instance()->Update();

	m_player.Update();

	EnemyManager::Instance()->Update();

	Collider::UpdateAllColliders();

	HitEffect::Update();

	//シャドウマップ用のライトカメラ、上からプレイヤーに追従
	static const float SHADOW_MAP_HEIGHT = 100.0f;
	auto playerPos = m_player.GetModelObj().lock()->m_transform.GetPos();
	auto shadowLightPos = playerPos;;
	shadowLightPos.y = SHADOW_MAP_HEIGHT;
	m_shadowMapDevice.SetPos(shadowLightPos);
	m_shadowMapDevice.SetTarget(playerPos);

}


void GameScene::OnDraw()
{
	BasicDraw::CountReset();

	auto cmdList = D3D12App::Instance()->GetCmdList();
	auto backBuff = D3D12App::Instance()->GetBackBuffRenderTarget();

	//デプスステンシル
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(backBuff->GetGraphSize());
	dsv->Clear(cmdList);

	//エミッシブマップ
	static std::shared_ptr<RenderTarget>emissiveMap = D3D12App::Instance()->GenerateRenderTarget(backBuff->GetDesc().Format, Color(0, 0, 0, 0), backBuff->GetGraphSize(), L"EmissiveMap");
	emissiveMap->Clear(cmdList);

	//深度マップ
	static std::shared_ptr<RenderTarget>depthMap = D3D12App::Instance()->GenerateRenderTarget(DXGI_FORMAT_R32_FLOAT, Color(0, 0, 0, 0), backBuff->GetGraphSize(), L"DepthMap");
	depthMap->Clear(cmdList);

	//現在のカメラ取得
	auto& nowCam = *GameManager::Instance()->GetNowCamera();

	//キューブマップに描き込む
	m_dynamicCubeMap->Clear();
	m_dynamicCubeMap->CopyCubeMap(m_staticCubeMap);
	m_dynamicCubeMap->DrawToCubeMap(m_ligMgr, { m_player.GetModelObj() });

	//シャドウマップに描き込み
	m_shadowMapDevice.DrawShadowMap({ m_player.GetModelObj() });

	//標準描画
	KuroEngine::Instance()->Graphics().SetRenderTargets({ backBuff,emissiveMap,depthMap }, dsv);

	//キューブマップ描画
	//staticCubeMap->Draw(nowCam);

	//影つき床
	m_shadowMapDevice.DrawShadowReceiver({ m_floorModel }, nowCam);

	//敵
	EnemyManager::Instance()->Draw(nowCam, m_dynamicCubeMap);

	//テスト用
	//BasicDraw::Draw(m_ligMgr, m_testObj, nowCam, m_staticCubeMap);

	//プレイヤー
	BasicDraw::Draw(m_ligMgr, m_player.GetModelObj(), nowCam, m_staticCubeMap);

	//HitEffect::Draw(nowCam);

	//DOF
	//m_dof.Draw(backBuff, depthMap);

	//ライトブルーム
	//m_lightBloomDevice.Draw(emissiveMap, backBuff);

	//当たり判定デバッグ描画
	static bool COLLIDER_DRAW = true;
	if (UsersInput::Instance()->ControllerOnTrigger(0, XBOX_BUTTON::BACK))
	{
		COLLIDER_DRAW = !COLLIDER_DRAW;
	}
	if (COLLIDER_DRAW)
	{
		Collider::DebugDrawAllColliders(nowCam);
	}
}

void GameScene::OnImguiDebug()
{
	GameManager::Instance()->ImGuiDebug(*UsersInput::Instance());
}

void GameScene::OnFinalize()
{
}