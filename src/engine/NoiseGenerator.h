#pragma once
#include<memory>
#include"Vec.h"
#include"D3D12Data.h"
#include<array>

enum NOISE_INTERPOLATION
{
	WAVELET, BLOCK, NOISE_INTERPOLATION_NUM
};

struct NoiseInitializer
{
	NOISE_INTERPOLATION m_interpolation = WAVELET;
	Vec2<int>m_split = { 16,16 };
	int m_contrast = 1;
	int m_octave = 1;
	float m_frequency = 1.0f;
	float m_persistance = 0.5f;
};

class NoiseGenerator
{
	static int s_perlinNoise2DId;
public:
	static void CountReset()
	{
		s_perlinNoise2DId = 0;
	}
	static void PerlinNoise2D(std::shared_ptr<TextureBuffer>DestTex, const NoiseInitializer& Config);
	static std::shared_ptr<TextureBuffer>PerlinNoise2D(const std::string& Name, const Vec2<int>& Size, const NoiseInitializer& Config, const DXGI_FORMAT& Format = DXGI_FORMAT_R32_FLOAT);

	//ノイズの補間方法名ゲッタ
	static const std::string& GetInterpolationName(const NOISE_INTERPOLATION& Interpolation)
	{
		static std::array<std::string, NOISE_INTERPOLATION_NUM>NAME =
		{
			"Wavelet","Block"
		};
		return NAME[Interpolation];
	}
};

