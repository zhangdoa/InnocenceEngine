#pragma once
#include "stdafx.h"
#include "Window.h"
#include "Input.h"
#include "RenderingEngine.h"
#include "AudioEngine.h"
#include "Time.h"
#include "Game.h"

class CoreEngine
{
public:
	CoreEngine(Game* game);
	~CoreEngine();
	void init(Game* game);
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