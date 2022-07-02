#include "GaussianBlur.h"
#include"KuroEngine.h"
#include"DrawFunc2D.h"

std::shared_ptr<ComputePipeline>GaussianBlur::X_BLUR_PIPELINE;	
std::shared_ptr<ComputePipeline>GaussianBlur::Y_BLUR_PIPELINE;	
std::shared_ptr<ComputePipeline>GaussianBlur::FINAL_PIPELINE;

void GaussianBlur::GeneratePipeline()
{
    if (!X_BLUR_PIPELINE)
    {
        std::vector<RootParam>rootParam =
        {
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"重みテーブル"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"テクスチャ情報"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ソース画像バッファ"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"描き込み先バッファ")
        };

        auto cs = D3D12App::Instance()->CompileShader("resource/engine/GaussianBlur.hlsl", "XBlur", "cs_5_0");
        X_BLUR_PIPELINE = D3D12App::Instance()->GenerateComputePipeline(cs, rootParam, { WrappedSampler(false, true) });
        cs = D3D12App::Instance()->CompileShader("resource/engine/GaussianBlur.hlsl", "YBlur", "cs_5_0");
        Y_BLUR_PIPELINE = D3D12App::Instance()->GenerateComputePipeline(cs, rootParam, { WrappedSampler(false, true) });
        cs = D3D12App::Instance()->CompileShader("resource/engine/GaussianBlur.hlsl", "Final", "cs_5_0");
        FINAL_PIPELINE = D3D12App::Instance()->GenerateComputePipeline(cs, rootParam, { WrappedSampler(false, true) });
    }
}

GaussianBlur::GaussianBlur(const Vec2<int>& Size, const DXGI_FORMAT& Format, const float& BlurPower)
{
    GeneratePipeline();

    //重みテーブル定数バッファ
    weightConstBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(float), NUM_WEIGHTS, nullptr, "GaussianBlur - weight");
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
    texInfoConstBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(TexInfo), 1, &texInfo, "GaussianBlur - TexInfo");

    //縦横ブラーの結果描画先
    xBlurResult = D3D12App::Instance()->GenerateTextureBuffer(texInfo.xBlurTexSize, Format, "HorizontalBlur");
    yBlurResult = D3D12App::Instance()->GenerateTextureBuffer(texInfo.yBlurTexSize, Format, "VerticalBlur");

    //最終合成結果描画先
    finalResult = D3D12App::Instance()->GenerateTextureBuffer(texInfo.sourceTexSize, Format, "GaussianBlur");
}

void GaussianBlur::SetBlurPower(const float& BlurPower)
{
    // 重みの合計を記録する変数を定義する
    float total = 0;

    // ここからガウス関数を用いて重みを計算している
    // ループ変数のxが基準テクセルからの距離
    for (int x = 0; x < NUM_WEIGHTS; x++)
    {
        weights[x] = expf(-0.5f * (float)(x * x) / BlurPower);
        total += 2.0f * weights[x];
    }

    // 重みの合計で除算することで、重みの合計を1にしている
    for (int i = 0; i < NUM_WEIGHTS; i++)
    {
        weights[i] /= total;
    }

    weightConstBuff->Mapping(&weights[0]);
}

void GaussianBlur::Excute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& CmdList, const std::shared_ptr<TextureBuffer>& SourceTex)
{
    const auto& sDesc = SourceTex->GetDesc();
    const auto& fDesc = finalResult->GetDesc();
    KuroFunc::ErrorMessage(sDesc.Width != fDesc.Width || sDesc.Height != fDesc.Height || sDesc.Format != fDesc.Format, "GaussianBlur", "Excute", "ソースとなるテクスチャ形式とガウシアンブラー形式が合いません\n");

    static const int DIV = 4;

    //Xブラー
    X_BLUR_PIPELINE->SetPipeline(CmdList);
    weightConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 0);
    texInfoConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 1);
    SourceTex->SetComputeDescriptorBuffer(CmdList, SRV, 2);
    xBlurResult->SetComputeDescriptorBuffer(CmdList, UAV, 3);
    CmdList->Dispatch(static_cast<UINT>(xBlurResult->GetDesc().Width / DIV), static_cast<UINT>(xBlurResult->GetDesc().Height / DIV), 1);

    //Yブラー
    Y_BLUR_PIPELINE->SetPipeline(CmdList);
    weightConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 0);
    texInfoConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 1);
    xBlurResult->SetComputeDescriptorBuffer(CmdList, SRV, 2);
    yBlurResult->SetComputeDescriptorBuffer(CmdList, UAV, 3);
    CmdList->Dispatch(static_cast<UINT>(yBlurResult->GetDesc().Width / DIV), static_cast<UINT>(yBlurResult->GetDesc().Height / DIV), 1);

    //最終結果合成
    FINAL_PIPELINE->SetPipeline(CmdList);
    weightConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 0);
    texInfoConstBuff->SetComputeDescriptorBuffer(CmdList, CBV, 1);
    yBlurResult->SetComputeDescriptorBuffer(CmdList, SRV, 2);
    finalResult->SetComputeDescriptorBuffer(CmdList, UAV, 3);
    CmdList->Dispatch(static_cast<UINT>(finalResult->GetDesc().Width / DIV), static_cast<UINT>(finalResult->GetDesc().Height / DIV), 1);
}

void GaussianBlur::Register(const std::shared_ptr<TextureBuffer>& SourceTex)
{
    const auto& sDesc = SourceTex->GetDesc();
    const auto& fDesc = finalResult->GetDesc();
    KuroFunc::ErrorMessage(sDesc.Width != fDesc.Width || sDesc.Height != fDesc.Height || sDesc.Format != fDesc.Format, "GaussianBlur", "Register", "ソースとなるテクスチャ形式とガウシアンブラー形式が合いません\n");

    static const int DIV = 4;
    Vec3<UINT>threadNum;

    //Xブラー
    KuroEngine::Instance().Graphics().SetComputePipeline(X_BLUR_PIPELINE);
    threadNum = { static_cast<UINT>(xBlurResult->GetDesc().Width / DIV), static_cast<UINT>(xBlurResult->GetDesc().Height / DIV), 1 };
    KuroEngine::Instance().Graphics().Dispatch(threadNum, { weightConstBuff,texInfoConstBuff,SourceTex,xBlurResult }, { CBV,CBV,SRV,UAV });

    //Yブラー
    KuroEngine::Instance().Graphics().SetComputePipeline(Y_BLUR_PIPELINE);
    threadNum = { static_cast<UINT>(yBlurResult->GetDesc().Width / DIV), static_cast<UINT>(yBlurResult->GetDesc().Height / DIV), 1 };
    KuroEngine::Instance().Graphics().Dispatch(threadNum, { weightConstBuff,texInfoConstBuff,xBlurResult,yBlurResult }, { CBV,CBV,SRV,UAV });

    //最終結果合成
    KuroEngine::Instance().Graphics().SetComputePipeline(FINAL_PIPELINE);
    threadNum = { static_cast<UINT>(finalResult->GetDesc().Width / DIV), static_cast<UINT>(finalResult->GetDesc().Height / DIV), 1 };
    KuroEngine::Instance().Graphics().Dispatch(threadNum, { weightConstBuff,texInfoConstBuff,yBlurResult,finalResult }, { CBV,CBV,SRV,UAV });
}

#include"KuroEngine.h"
void GaussianBlur::DrawResult(const AlphaBlendMode& AlphaBlend)
{
    KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() });
    DrawFunc2D::DrawExtendGraph2D({ 0,0 }, WinApp::Instance()->GetExpandWinSize(), finalResult, AlphaBlend);
}
