#include "CoreEngine.h"


CoreEngine::CoreEngine(Game* game)
{
	init(game);
}


CoreEngine::~CoreEngine()
{
	shutdown();
}

void CoreEngine::init(Game* game)
{
	isRunning = true;
	frameTime = 1.0f / frameRate;

	_time = new Time();
	_window = new Window();
	_input = new Input();
	_input->setWindow(_window);
	_renderingEngine = new RenderingEngine();
	_audioEngine = new AudioEngine();
	_game = game;



	/*_audioEngine->playSound("D:/WwiseOriginalFiles/NewSourceFiles/20170418/Item_Ride_Egg_Break.wav");
	_audioEngine->playSound("F:/Installation Files/Music/TesseracT - Altered State (2013) [FLAC]/test.wav");*/

}

void CoreEngine::update()
{
	while (isRunning) {

		_window->update();
		_input->update();
		_audioEngine->update();

		bool render = false;

		int frames = 0;
		double frameConter = 0;
		double unprocessedTime = 0;
		double passedTime = 0;

		passedTime = _time->getDelta();
		unprocessedTime += passedTime;
		frameConter += passedTime;



		while (unprocessedTime > frameTime)
		{
			render = true;

			unprocessedTime -= frameTime;

			if (frameConter >= 1.0) {

				frames = 0;
				frameConter = 0;
			}
		}

		if (render)
		{
			frames++;
			_game->render(_renderingEngine);
		}
		else
		{
			try
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			catch (const std::exception&)
			{

			}
		}


		

	}

	isRunning = false;
	shutdown();


}

void CoreEngine::shutdown() {
	delete _window;
	delete _input;
	delete _renderingEngine;
	delete _audioEngine;
	std::this_thread::sleep_for(std::chrono::seconds(3));
}
