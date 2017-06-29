#pragma once
#include "stdafx.h"
#include "Window.h"
#include "Input.h"
#include "RenderingEngine.h"
#include "AudioEngine.h"
#include "Game.h"

class Time {
public:

	Time();
	~Time();

	const double getTime();
	const double getDelta();

private:
	double delta;
};

class CoreEngine
{
public:
	CoreEngine();
	~CoreEngine();
	void init();
	void update();
	void shutdown();
	bool isRunning;

private:
	double frameTime = 0;
	const int frameRate = 60;

	Window* _window;
	Input* _input;
	RenderingEngine* _renderingEngine;
	AudioEngine* _audioEngine;
	Time* _time;
	Game* _game;
};