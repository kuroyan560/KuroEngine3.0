#include "RadialBlur.h"
#include"KuroEngine.h"

std::shared_ptr<ComputePipeline>RadialBlur::RADIAL_PIPELINE;

void RadialBlur::GeneratePipeline()
{
	if (!RADIAL_PIPELINE)
	{
        std::vector<RootParam>rootParam =
        {
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ブラー情報"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ソース画像バッファ"),
            RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"描き込み先バッファ")
        };

        auto cs = D3D12App::Instance()->CompileShader("resource/engine/RadialBlur.hlsl", "CSmain", "cs_5_0");
        RADIAL_PIPELINE = D3D12App::Instance()->GenerateComputePipeline(cs, rootParam, { WrappedSampler(false, true) });
	}
}

RadialBlur::RadialBlur(const Vec2<int>& Size, const DXGI_FORMAT& Format, const float& BlurPower, const Vec2<float>& CenterUV)
{
    GeneratePipeline();

    //重みテーブル定数バッファ
    constData.texSize = Size;
    constData.blurPower = BlurPower;
    constData.centerUV = CenterUV;
    constBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(constData), 1, &constData, "RadialBlur - ConstBuffer");

    //最終合成結果描画先
    finalResult = D3D12App::Instance()->GenerateTextureBuffer(Size, Format, "RadialBlur");
}

void RadialBlur::SetBlurPower(const float& BlurPower)
{
    constData.blurPower = BlurPower;
    constBuff->Mapping(&constData);
}

void RadialBlur::SetCenterUV(const Vec2<float>& CenterUV)
{
    constData.centerUV = CenterUV;
    constBuff->Mapping(&constData);
}

void RadialBlur::Excute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& CmdList, const std::shared_ptr<TextureBuffer>& SourceTex)
{
    const auto& sDesc = SourceTex->GetDesc();
    const auto& fDesc = finalResult->GetDesc();
    KuroFunc::ErrorMessage(sDesc.Width != fDesc.Width || sDesc.Height != fDesc.Height || sDesc.Format != fDesc.Format, "GaussianBlur", "Excute", "ソースとなるテクスチャ形式とガウシアンブラー形式が合いません\n");

    static const int DIV = 4;

    //方向ブラー
    RADIAL_PIPELINE->SetPipeline(CmdList);
    constBuff->SetComputeDescriptorBuffer(CmdList, CBV, 0);
    SourceTex->SetComputeDescriptorBuffer(CmdList, SRV, 1);
    finalResult->SetComputeDescriptorBuffer(CmdList, UAV, 2);
    CmdList->Dispatch(static_cast<UINT>(finalResult->GetDesc().Width / DIV), static_cast<UINT>(finalResult->GetDesc().Height / DIV), 1);
}
