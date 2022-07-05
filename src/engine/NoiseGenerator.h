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
	NOISE_INTERPOLATION interpolation = WAVELET;
	Vec2<int>split = { 16,16 };
	int contrast = 1;
	int octave = 1;
	float frequency = 1.0f;
	float persistance = 0.5f;
};

class NoiseGenerator
{
	static int PERLIN_NOISE_ID_2D;
public:
	static void CountReset()
	{
		PERLIN_NOISE_ID_2D = 0;
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

