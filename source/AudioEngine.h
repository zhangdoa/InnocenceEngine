#pragma once
#include "stdafx.h"

class AudioEngine {

public:
	AudioEngine();
	~AudioEngine();

	void init();
	void shutdown();
	void update();
	bool isRunning;

	ALuint playSound(const char* fileName);
	void stopSound(ALuint source);
	void pauseSound(ALuint source);

private:
	ALuint loadSound(const char* fileName);
	static void reportError(void);
	std::vector<ALuint> sources;


};


