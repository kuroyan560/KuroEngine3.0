#include"Fps.h"
#include <profileapi.h>

static const float MILL_SECOND = 1.0 / 1000;

Fps::Fps(const int& FrameRate) : m_frameRate(FrameRate), m_minFrameTime(1.0f / (float)m_frameRate)
{
	// メインループに入る前に精度を取得しておく
	if (QueryPerformanceFrequency(&m_timeFreq) == FALSE) { // この関数で0(FALSE)が帰る時は未対応
		// そもそもQueryPerformanceFrequencyが使えない様な(古い)PCではどうせ色々キツイだろうし
	}
	// 1度取得しておく(初回計算用)
	QueryPerformanceCounter(&m_timeStart);
}

void Fps::Update()
{
	//Sleep(MILL_SECOND * 20);	//20ms待つ

	// 今の時間を取得
	QueryPerformanceCounter(&m_timeEnd);
	// (今の時間 - 前フレームの時間) / 周波数 = 経過時間(秒単位)
	m_frameTime = static_cast<float>(m_timeEnd.QuadPart - m_timeStart.QuadPart) / static_cast<float>(m_timeFreq.QuadPart);

	if (m_frameTime < m_minFrameTime) { // 時間に余裕がある
		// ミリ秒に変換
		DWORD sleepTime = static_cast<DWORD>((m_minFrameTime - m_frameTime) * 1000);

		timeBeginPeriod(1); // 分解能を上げる(こうしないとSleepの精度はガタガタ)
		Sleep(sleepTime);   // 寝る
		timeEndPeriod(1);   // 戻す

		// 次週に持ち越し(こうしないとfpsが変になる?)
	}

	if (m_frameTime > 0.0) { // 経過時間が0より大きい(こうしないと下の計算でゼロ除算になると思われ)
		m_fps = (m_fps * 0.99f) + (0.01f / m_frameTime); // 平均fpsを計算
		m_timeStart = m_timeEnd; // 入れ替え
	}
}