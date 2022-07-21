#pragma once
#include<xaudio2.h>
#pragma comment(lib,"xaudio2.lib")

#include<wrl.h>

#include<fstream>
#include<assert.h>

#include<list>

#include<string>

#include<vector>

class AudioApp
{
private:
	static AudioApp* s_instance;
public:
	static AudioApp* Instance()
	{
		if (s_instance == nullptr)
		{
			printf("AudioAppのインスタンスを呼び出そうとしましたがnullptrでした\n");
			assert(0);
		}
		return s_instance;
	}
private:
	//チャンクヘッダ
	struct Chunk
	{
		char m_id[4];		//チャンク毎のID
		int32_t m_size;	//チャンクのサイズ
	};

	//RTFFヘッダチャンク
	struct RiffHeader
	{
		Chunk m_chunk;	//"RIFF"
		char m_type[4];	//"WAVE"
	};

	//FMTチャンク
	struct FormatChunck
	{
		Chunk m_chunk;	//"fmt "
		WAVEFORMAT m_fmt;	//波形フォーマット
	};

	Microsoft::WRL::ComPtr<IXAudio2>m_xAudio2;
	IXAudio2MasteringVoice* m_masterVoice;

	class XAudio2VoiceCallback : public IXAudio2VoiceCallback
	{
	public:
		virtual ~XAudio2VoiceCallback() {};
		//ボイス処理パスの開始時
		STDMETHOD_(void, OnVoiceProcessingPassStart)(THIS_ UINT32 BytesRequired) {};
		//ボイス処理パスの終了時
		STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS) {};
		//バッファストリームの再生が終了したとき
		STDMETHOD_(void, OnStreamEnd) (THIS) {};
		//バッファの使用開始時
		STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext) {};
		//バッファの末尾に達した時
		STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext){	};
		//再生がループ位置に達した時
		STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext) {};
		//ボイスの実行エラー時
		STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error) {};
	};
	XAudio2VoiceCallback m_voiceCallback;

	class AudioData
	{
	public:
		std::string m_filePass;
		RiffHeader m_riff;
		FormatChunck m_format;
		Chunk m_data;
		char* m_pBuffer = nullptr;
		IXAudio2SourceVoice* m_pSourceVoice = nullptr;
		float m_volume = 1.0f;
		bool m_loop = false;
		bool m_playTrigger = false;

		AudioData(std::string FilePass)
			:m_filePass(FilePass) {};
		void Unload();
	};

	struct PlayAudioArray
	{
		std::vector<int>m_handles;
		int m_nowIdx = 0;
		PlayAudioArray(const std::vector<int>& Handles) :m_handles(Handles) {}
	};
	std::list<AudioData>m_audios;
	std::vector<PlayAudioArray>m_playHandleArray;

public:
	AudioApp();
	~AudioApp();

	void Update();
	bool NowPlay(const int& Handle);
	void ChangeVolume(const int& Handle, float Volume);
	float GetVolume(const int& Handle);
	int LoadAudio(std::string FileName, const float& Volume = 1.0f);
	int PlayWave(const int& Handle, bool LoopFlag = false);
	int PlayWaveArray(const std::vector<int>& Handles)	//複数の音声を同フレームで再生しないよう、順番に再生
	{
		if (Handles.empty())return 0;
		m_playHandleArray.emplace_back(Handles);
		return PlayWave(Handles[0]);
	}
	void StopWave(const int& Handle);
};