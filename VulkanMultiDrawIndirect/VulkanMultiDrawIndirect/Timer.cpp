#include "Timer.h"



Timer::Timer() : _secondsPerCount(0.0), _deltaTime(-1.0), _baseTime(0), _pausedTime(0), _prevTime(0), _currTime(0), _stopped(false), _mspf(0.0), _fps(0), _frameCount(0), _timeElapsed(0.0f), _stopTime(0)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)& countsPerSec);
	auto countsPerMS = countsPerSec / 1000.0;
	_secondsPerCount = 1.0 / (double)countsPerSec;
	_msPerCount = 1.0 / (double)countsPerMS;
}


Timer::~Timer()
{
}

const float Timer::DeltaTime() const
{
	return (float)_deltaTime;
}

const float Timer::TotalTime() const
{
	if (_stopped)
		return (float)(((_stopTime - _pausedTime) - _baseTime)*_secondsPerCount);
	else
		return (float)(((_currTime - _pausedTime) - _baseTime)*_secondsPerCount);
}
const float Timer::TotalTimeMS() const
{
	if (_stopped)
		return (float)(((_stopTime - _pausedTime) - _baseTime)*_msPerCount);
	else
		return (float)(((_currTime - _pausedTime) - _baseTime)*_msPerCount);
}
const unsigned int Timer::GetFps() const
{
	return _fps;
}

const float Timer::GetMspf() const
{
	return (float)_mspf;
}

void Timer::Reset()
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)& countsPerSec);
	_secondsPerCount = 1.0 / (double)countsPerSec;
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	_baseTime = currTime;
	_prevTime = currTime;
	_stopTime = 0;
	_stopped = false;
	_frameCount = 0;
	_timeElapsed = 0.0f;
	_mspf = 0.0;
	_fps = 0;
	return void();
}

void Timer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (_stopped)
	{
		_pausedTime += startTime - _stopTime;

		_prevTime = startTime;

		_stopTime = 0;
		_stopped = false;
	}
	return void();
}

void Timer::Stop()
{
	if (!_stopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		_stopTime = currTime;
		_stopped = true;
	}
	return void();
}

void Timer::Tick()
{
	if (_stopped)
	{
		_deltaTime = 0.0;
		return;
	}

	// Get time this frame
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	_currTime = currTime;

	// Time diff between this and previous frame.
	_deltaTime = (_currTime - _prevTime)*_secondsPerCount;

	// Prepare for next frame
	_prevTime = _currTime;

	if (_deltaTime < 0.0)
		_deltaTime = 0.0;



	_frameCount++;

	if ((TotalTime() - _timeElapsed) >= 1.0f)
	{
		_fps = _frameCount;
		_mspf = 1000.0f / (float)_fps;


		_frameCount = 0;
		_timeElapsed += 1.0f;
	}
	return void();
}