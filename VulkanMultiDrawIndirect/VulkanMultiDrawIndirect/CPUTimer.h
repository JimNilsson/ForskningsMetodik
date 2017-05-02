#ifndef _CPUTTIMER_H_
#define _CPUTTIMER_H_

#pragma once
#include "Timer.h"
#include <map>

class CPUTimer
{

public:
	CPUTimer();
	void TimeStart(const std::string& name);
	void TimeEnd(const std::string& name);
	const float GetTime(const std::string& name);
	const float GetTime();
	const float GetAVGTPF(const std::string& name);
private:

	struct ProfileData
	{
		BOOL QueryStarted;
		BOOL QueryFinished;

		Timer* timer;
		float _frameTime = 0.0f;
		float _ltime = 0.0f;
		float _timeElapsed = 0.0f;
		uint32_t _frameCount = 0;
		ProfileData() : QueryStarted(FALSE), QueryFinished(TRUE)
		{
			timer = nullptr;
		}
		~ProfileData()
		{
			delete timer;
		}
	};

	typedef std::map<std::string, ProfileData> ProfileMap;

	ProfileMap profiles;
	UINT64 currFrame = 0;
	Timer _timer;

};

#endif