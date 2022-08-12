#include <Windows.h>
#include <iostream>
#include <inttypes.h>
#include <vector>

#include "AudioRecorder.h"
#include "Renderer.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    AudioRecorder recorder;
    recorder.Init();
    recorder.Start();

    Renderer renderer;
    renderer.Init(hInstance);
    
    while (true)
    {
        std::vector<float> audioSamples;
        recorder.GetWaveform(100, audioSamples);

        float maxValue = 0;
        for (int i = 0; i < audioSamples.size(); ++i)
        {
            if (abs(audioSamples[i]) > maxValue)
            {
                maxValue = audioSamples[i];
            }
        }

        std::cout << audioSamples[0] << std::endl;
        Sleep(50);


        renderer.Render();
    }

	return 0;
}

