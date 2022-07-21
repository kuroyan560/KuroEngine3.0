#include "GaussianBlur.h"
#include"KuroEngine.h"
#include"DrawFunc2D.h"

std::shared_ptr<ComputePipeline>GaussianBlur::s_xBlurPipeline;	
std::shared_ptr<ComputePipeline>GaussianBlur::s_yBlurPipeline;	
std::shared_ptr<ComputePipeline>GaussianBlur::s_finalPipeline;

void GaussianBlur::GeneratePipeline()
{
    if (!s_xBlurPipeline)
    {
        std::vector<RootParam>rootParam =
        {
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"重みテーブル"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"テクスチャ情報"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ソース画像バッファ"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"描き込み先バッファ")
        };

        auto cs = D3D12App::Instance()->CompileShader("resource/engine/GaussianBlur.hlsl", "XBlur", "cs_6_4");
        s_xBlurPipeline = D3D12App::Instance()->GenerateComputePipeline(cs, rootParam, { WrappedSampler(false, true) });
        cs = D3D12App::Instance()->CompileShader("resource/engine/GaussianBlur.hlsl", "YBlur", "cs_6_4");
        s_yBlurPipeline = D3D12App::Instance()->GenerateComputePipeline(cs, rootParam, { WrappedSampler(false, true) });
        cs = D3D12App::Instance()->CompileShader("resource/engine/GaussianBlur.hlsl", "Final", "cs_6_4");
        s_finalPipeline = D3D12App::Instance()->GenerateComputePipeline(cs, rootParam, { WrappedSampler(false, true) });
    }
}

GaussianBlur::GaussianBlur(const Vec2<int>& Size, const DXGI_FORMAT& Format, const float& BlurPower)
{
    GeneratePipeline();

    //重みテーブル定数バッファ
    m_weightConstBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(float), s_weightNum, nullptr, "GaussianBlur - weight");
    SetBlurPower(BlurPower);

    //テクスチャ情報
    struct TexInfo
    {
        Vec2<int>sourceTexSize;
        Vec2<int>xBlurTexSize;
        Vec2<int>yBlurTexSize;
        Vec2<int>pad;
    }texInfo;
    texInfo.sourceTexSize = Size;
    texInfo.xBlurTexSize = { Size.x / 2,Size.y };
    texInfo.yBlurTexSize = { Size.x / 2,Size.y / 2 };
    m_texInfoConstBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(TexInfo), 1, &texInfo, "GaussianBlur - TexInfo");

    //縦横ブラーの結果描画先
    m_xBlurResult = D3D12App::Instance()->GenerateTextureBuffer(texInfo.xBlurTexSize, Format, "HorizontalBlur");
    m_yBlurResult = D3D12App::Instance()->GenerateTextureBuffer(texInfo.yBlurTexSize, Format, "VerticalBlur");

    //最終合成結果描画先
    m_finalResult = D3D12App::Instance()->GenerateTextureBuffer(texInfo.sourceTexSize, Format, "GaussianBlur");
}

void GaussianBlur::SetBlurPower(const float& BlurPower)
{
    // 重みの合計を記録する変数を定義する
    float total = 0;

    // ここからガウス関数を用いて重みを計算している
    // ループ変数のxが基準テクセルからの距離
    for (int x = 0; x < s_weightNum; x++)
    {
        m_weights[x] = expf(-0.5f * (float)(x * x) / BlurPower);
        total += 2.0f * m_weights[x];
    }

    // 重みの合計で除算することで、重みの合計を1にしている
    for (int i = 0; i < s_weightNum; i++)
    {
        m_weights[i] /= total;
    }

    m_weightConstBuff->Mapping(&m_weights[0]);
}

void GaussianBlur::Execute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& CmdList, const std::shared_ptr<TextureBuffer>& SourceTex)
{
    const auto& sDesc = SourceTex->GetDesc();
    const auto& fDesc = m_finalResult->GetDesc();
    assert(sDesc.Width == fDesc.Width && sDesc.Height == fDesc.Height && sDesc.Format == fDesc.Format);

    static const int DIV = 4;

    //Xブラー
    s_xBlurPipeline->SetPipeline(CmdList);
    m_weightConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 0);
    m_texInfoConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 1);
    SourceTex->SetComputeDescriptorBuffer(CmdList, SRV, 2);
    m_xBlurResult->SetComputeDescriptorBuffer(CmdList, UAV, 3);
    CmdList->Dispatch(static_cast<UINT>(m_xBlurResult->GetDesc().Width / DIV), static_cast<UINT>(m_xBlurResult->GetDesc().Height / DIV), 1);

    //Yブラー
    s_yBlurPipeline->SetPipeline(CmdList);
    m_weightConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 0);
    m_texInfoConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 1);
    m_xBlurResult->SetComputeDescriptorBuffer(CmdList, SRV, 2);
    m_yBlurResult->SetComputeDescriptorBuffer(CmdList, UAV, 3);
    CmdList->Dispatch(static_cast<UINT>(m_yBlurResult->GetDesc().Width / DIV), static_cast<UINT>(m_yBlurResult->GetDesc().Height / DIV), 1);

    //最終結果合成
    s_finalPipeline->SetPipeline(CmdList);
    m_weightConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 0);
    m_texInfoConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 1);
    m_yBlurResult->SetComputeDescriptorBuffer(CmdList, SRV, 2);
    m_finalResult->SetComputeDescriptorBuffer(CmdList, UAV, 3);
    CmdList->Dispatch(static_cast<UINT>(m_finalResult->GetDesc().Width / DIV), static_cast<UINT>(m_finalResult->GetDesc().Height / DIV), 1);
}

void GaussianBlur::Register(const std::shared_ptr<TextureBuffer>& SourceTex)
{
    const auto sourceSize = SourceTex->GetGraphSize();
    const auto resultSize = m_finalResult->GetGraphSize();
    assert(sourceSize == resultSize);

    static const int DIV = 4;
    Vec3<int>threadNum;

    //Xブラー
    KuroEngine::Instance().Graphics().SetComputePipeline(s_xBlurPipeline);
    threadNum = { m_xBlurResult->GetGraphSize().x / DIV,m_xBlurResult->GetGraphSize().y / DIV, 1 };
    KuroEngine::Instance().Graphics().Dispatch(threadNum, { m_weightConstBuff,m_texInfoConstBuff,SourceTex,m_xBlurResult }, { CBV,CBV,SRV,UAV });

    //Yブラー
    KuroEngine::Instance().Graphics().SetComputePipeline(s_yBlurPipeline);
    threadNum = { m_yBlurResult->GetGraphSize().x / DIV, m_yBlurResult->GetGraphSize().y / DIV, 1 };
    KuroEngine::Instance().Graphics().Dispatch(threadNum, { m_weightConstBuff,m_texInfoConstBuff,m_xBlurResult,m_yBlurResult }, { CBV,CBV,SRV,UAV });

    //最終結果合成
    KuroEngine::Instance().Graphics().SetComputePipeline(s_finalPipeline);
    threadNum = { m_finalResult->GetGraphSize().x / DIV, m_finalResult->GetGraphSize().y / DIV, 1 };
    KuroEngine::Instance().Graphics().Dispatch(threadNum, { m_weightConstBuff,m_texInfoConstBuff,m_yBlurResult,m_finalResult }, { CBV,CBV,SRV,UAV });
}

#include"KuroEngine.h"
void GaussianBlur::DrawResult(const AlphaBlendMode& AlphaBlend)
{
    KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() });
    DrawFunc2D::DrawExtendGraph2D({ 0,0 }, WinApp::Instance()->GetExpandWinSize(), m_finalResult, AlphaBlend);
}
