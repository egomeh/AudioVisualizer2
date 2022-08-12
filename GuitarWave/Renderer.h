#pragma once

#include <Windows.h>
#include <string>


class Renderer
{
public:

	Renderer();

	void Init(HINSTANCE hInstance);

	void Render();

	bool running;

private:

	void LoadOpenGLFunctions();

	std::wstring className;
	HDC m_DeviceContext;
};

