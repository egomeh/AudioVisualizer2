#include "AudioRecorder.h"

#include <Windows.h>
#include "mmeapi.h"
#include <iostream>
#include <vector>

#pragma comment(lib, "Winmm.lib")

AudioRecorder::AudioRecorder()
{
    ApplySettings(GetDefaultSettings());
}

bool AudioRecorder::Init()
{
	if (!InitializeCriticalSectionAndSpinCount(&bufferCS, 0x4000))
		return false;

	ZeroMemory(&waveFormat, sizeof(waveFormat));
	waveFormat.nChannels = settings.channelCount;
	waveFormat.cbSize = 0;
	waveFormat.nSamplesPerSec = settings.sampleRate;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.wBitsPerSample = settings.bitsPerSample;
	waveFormat.nBlockAlign = (settings.channelCount * waveFormat.wBitsPerSample) / 8;
	waveFormat.nAvgBytesPerSec = settings.sampleRate * waveFormat.nBlockAlign;

	if (MMSYSERR_NOERROR != waveInOpen(&microphoneHandle, WAVE_MAPPER, &waveFormat, (DWORD_PTR)AudioRecorder::waveInCallback, (DWORD_PTR)this, CALLBACK_FUNCTION))
	{
		return false;
	}

	waveHeaders.resize(settings.bufferCount);

	for (int i = 0; i < settings.bufferCount; i++)
	{
		WAVEHDR header;
		ZeroMemory(&header, sizeof(header));

		const int dataSize = ((settings.bitsPerSample / 8) * settings.sampleRate) / settings.bufferCount;
		header.lpData = (LPSTR)new unsigned char[dataSize];
		header.dwBufferLength = dataSize;
		header.dwBytesRecorded = 0;
		header.dwUser = 0;
		header.dwFlags = 0;
		header.dwLoops = 0;
		header.dwUser = i;

		waveHeaders[i] = header;
		MMRESULT prepareResult = waveInPrepareHeader(microphoneHandle, &waveHeaders[i], sizeof(WAVEHDR));
		MMRESULT addBufferResult = waveInAddBuffer(microphoneHandle, &waveHeaders[i], sizeof(WAVEHDR));
	}

	return true;
}

void AudioRecorder::Start()
{
	waveInStart(microphoneHandle);
}

void AudioRecorder::Stop()
{
	waveInStop(microphoneHandle);
}

void AudioRecorder::ApplySettings(const AudioRecorderSettings& inSettings)
{
	settings = inSettings;
    rawSamples.resize(settings.sampleRate * (settings.bitsPerSample / 8), 0);
	rawSamplesPointer = 0;
}

AudioRecorderSettings AudioRecorder::GetDefaultSettings()
{
	AudioRecorderSettings defaultSettings;

	defaultSettings.bufferCount = 20;
	defaultSettings.channelCount = 1;
	defaultSettings.sampleRate = 44100;
	defaultSettings.bitsPerSample = 16;

	return defaultSettings;
}

void CALLBACK AudioRecorder::waveInCallback(HWAVEIN hWaveIn, UINT uMesg, DWORD_PTR userData, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMesg == WIM_DATA)
	{
		AudioRecorder* recorder = (AudioRecorder*)userData;

		EnterCriticalSection(&recorder->bufferCS);

		WAVEHDR* header = (WAVEHDR*)dwParam1;
		DWORD_PTR headerID = header->dwUser;

		const size_t remainingBufferSize = recorder->rawSamples.size() - recorder->rawSamplesPointer;
		const size_t bytesToDirectCopy = min(header->dwBufferLength, remainingBufferSize);
		const size_t bytesToWrapAround = max(0, header->dwBufferLength - (int64_t)remainingBufferSize);

		memcpy(recorder->rawSamples.data() + recorder->rawSamplesPointer, header->lpData, bytesToDirectCopy);

		if (bytesToWrapAround > 0)
			memcpy(recorder->rawSamples.data(), header->lpData + (header->dwBufferLength - bytesToWrapAround), bytesToWrapAround);

		recorder->rawSamplesPointer += bytesToDirectCopy;
		recorder->rawSamplesPointer = recorder->rawSamplesPointer % recorder->rawSamples.size();

		waveInPrepareHeader(hWaveIn, &recorder->waveHeaders[headerID], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &recorder->waveHeaders[headerID], sizeof(WAVEHDR));

		LeaveCriticalSection(&recorder->bufferCS);
	}
}

void AudioRecorder::GetWaveform(const int timeWindowMs, std::vector<float>& data)
{
	EnterCriticalSection(&bufferCS);

	const int nSamples = (settings.sampleRate / 1'000) * timeWindowMs;

	data.resize(nSamples, 0);
	float floatMaxValue = 32'768.0f;

	for (int i = 0; i < nSamples; ++i)
	{
		const int dataOffset = (int)(rawSamplesPointer + (settings.bitsPerSample / 8) * i) % rawSamples.size();
		float sampleFloatValue = *(uint16_t*)(rawSamples.data() + dataOffset);
		sampleFloatValue += floatMaxValue;
		sampleFloatValue /= floatMaxValue * 2.0f;
		sampleFloatValue -= 0.5f;
		data[i] = sampleFloatValue / floatMaxValue;
	}

	LeaveCriticalSection(&bufferCS);
}
