#include "CoreEngine.h"

Time::Time() {
}
Time::~Time() {}

const double Time::getTime() {
	return 1.0;
}

const double Time::getDelta()
{

	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::duration<float, std::milli> duration;

	static clock::time_point start = clock::now();
	duration elapsed = clock::now() - start;
	return elapsed.count();

}

CoreEngine::CoreEngine()
{
	init();
}


CoreEngine::~CoreEngine()
{
	shutdown();
}

void CoreEngine::init()
{
	isRunning = true;
	frameTime = 1.0f / frameRate;

	_time = new Time();
	_window = new Window();
	_input = new Input();
	_input->setWindow(_window);
	_renderingEngine = new RenderingEngine();
	_audioEngine = new AudioEngine();
	_game = new Game();
	_game->setRenderingEngine(_renderingEngine);
	_renderingEngine->setMainCamera(_game->getCameraComponent());


	fprintf(stdout, "Core Engine has been initialized.\n");
	/*_audioEngine->playSound("D:/WwiseOriginalFiles/NewSourceFiles/20170418/Item_Ride_Egg_Break.wav");
	_audioEngine->playSound("F:/Installation Files/Music/TesseracT - Altered State (2013) [FLAC]/test.wav");*/

}

void CoreEngine::update()
{
	int frames = 0;
	double frameConter = 0;
	double unprocessedTime = 0;
	double passedTime = 0;

	while (isRunning) {

		passedTime = _time->getDelta();
		unprocessedTime += passedTime;
		frameConter += passedTime;

		while (unprocessedTime > frameTime)
		{
			//fprintf(stdout, "Core Engine is updating.\n");
			unprocessedTime -= frameTime;

			_game->input(frameTime);
			_input->update();
			_game->update(frameTime);

			//fprintf(stdout, "Core Engine is rendering.\n");
			_window->render();
			_game->render();
			_audioEngine->update();
			frames++;

			if (frameConter >= 1.0) {
				frames = 0;
				frameConter = 0;
			}
		}
		//fprintf(stdout, "Core Engine is sleeping.\n");
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	isRunning = false;
	shutdown();


}

void CoreEngine::shutdown() {
	delete _window;
	delete _input;
	delete _renderingEngine;
	delete _audioEngine;
	fprintf(stdout, "Core Engine has been destroyed.\n");
	std::this_thread::sleep_for(std::chrono::seconds(3));
}
