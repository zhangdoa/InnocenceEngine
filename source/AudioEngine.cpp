#include "AudioEngine.h"




AudioEngine::AudioEngine()
{
	init();
}

AudioEngine::~AudioEngine(void)
{
	shutdown();
}

void AudioEngine::init()
{
	isRunning = true;
	//ALboolean initStatus = alutInit(argc, argv);
	if (!alutInit(0, nullptr))
	{
		reportError();
	}

	fprintf(stdout, "Audio Engine has been initialized.\n");
}

void AudioEngine::update() {


	//for (int i = 0; i!=sources.size(); i++)
	//{
	//	ALuint state = 0;
	//	alSourcei(sources.at(i), AL_SOURCE_STATE, state);
	//	isRunning = (state == AL_PLAYING ? true : false);	
	//	
	//}
}

void AudioEngine::shutdown(void)
{
	isRunning = false;
	if (alutExit())
	{
		reportError();
	}


	fprintf(stdout, "AudioEngine has been destroyed.\n");

}

ALuint AudioEngine::loadSound(const char * fileName)
{
	fprintf(stdout, fileName);
	fprintf(stdout, " has been loaded.\n");
	return alutCreateBufferFromFile(fileName);
}



ALuint AudioEngine::playSound(const char * fileName)
{

	ALuint source;
	sources.push_back(source);
	alGenSources(1, &source);
	ALuint buffer;
	buffer = loadSound(fileName);
	alSourcei(source, AL_BUFFER, buffer);
	alSourcePlay(source);

	fprintf(stdout, fileName);
	fprintf(stdout, " has been played.\n");
	return source;
}

void AudioEngine::stopSound(ALuint source)
{
	alSourceStop(source);

}

void AudioEngine::pauseSound(ALuint source)
{
	alSourcePause(source);
}

void AudioEngine::reportError(void)
{
	fprintf(stderr, "ALUT error: %s\n",
		alutGetErrorString(alutGetError()));
	exit(EXIT_FAILURE);
}
