#pragma once

#include <Windows.h>
#include <inttypes.h>
#include <vector>

struct AudioRecorderSettings
{
	int sampleRate;
	int channelCount;
	int bufferCount;
	int bitsPerSample;
};

class AudioRecorder
{
public:
	AudioRecorder();

	bool Init();

	void Start();
	void Stop();

	void ApplySettings(const AudioRecorderSettings& settings);
	AudioRecorderSettings GetDefaultSettings();

	static void CALLBACK waveInCallback(HWAVEIN hWaveIn, UINT uMesg, DWORD_PTR userData, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

	void GetWaveform(const int timeWindowMs, std::vector<float>& data);

private:

	AudioRecorderSettings settings;

	std::vector<unsigned char> rawSamples;
	size_t rawSamplesPointer;

	HWAVEIN microphoneHandle;
	std::vector<WAVEHDR> waveHeaders;
	WAVEFORMATEX waveFormat;

	CRITICAL_SECTION bufferCS;
};



