#ifndef _TIMER_H_
#define _TIMER_H_

#pragma once
//////////////
// Includes //
//////////////
#include <Windows.h>
class Timer
{
public:
	Timer();
	~Timer();

	const float DeltaTime()const;
	const float TotalTime()const;
	const float TotalTimeMS()const;
	const unsigned int GetFps()const;
	const float GetMspf()const;
	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double _secondsPerCount;
	double _msPerCount;
	double _deltaTime;
	__int64 _pausedTime;
	__int64 _baseTime;
	__int64 _stopTime;
	__int64 _prevTime;
	__int64 _currTime;
	double _mspf;
	unsigned int _fps;
	unsigned int _frameCount;
	float _timeElapsed;

	bool _stopped;
};

#endif