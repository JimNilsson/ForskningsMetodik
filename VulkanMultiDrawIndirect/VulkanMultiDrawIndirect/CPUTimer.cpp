#include "CPUTimer.h"


CPUTimer::CPUTimer()
{
	_timer.Start();
	_timer.Reset();
}

void CPUTimer::TimeStart(const std::string & name)
{
	ProfileData& profileData = profiles[name];
	if (profileData.QueryStarted == TRUE)
		return;
	if (profileData.QueryFinished == FALSE)
		return;

	if (profileData.timer == nullptr)
	{
		profileData.timer = new Timer;

	}
	profileData.timer->Reset();
	profileData.timer->Start();
	profileData.timer->Tick();
	profileData.QueryStarted = TRUE;
	profileData.QueryFinished = FALSE;
}

void CPUTimer::TimeEnd(const std::string & name)
{
	auto i = profiles.find(name);
	if (i == profiles.end())
		return;
	ProfileData& profileData = i->second;
	_ASSERT(profileData.QueryStarted == TRUE);
	_ASSERT(profileData.QueryFinished == FALSE);

	profileData.timer->Tick();
	profileData.timer->Stop();

	profileData.QueryStarted = FALSE;
	profileData.QueryFinished = TRUE;
}

const float CPUTimer::GetTime(const std::string & name)
{
	auto i = profiles.find(name);
	if (i == profiles.end())
		return 0.0f;
	ProfileData& profile = i->second;
	if (profile.QueryFinished == TRUE)
	{

	//	profile.QueryFinished = FALSE;

		if (profile.timer != nullptr)
		{

			// Get the query data
			UINT64 startTime = 0;

			float time = profile.timer->TotalTimeMS();
			return time;
		}
	}

	return 0.0f;
}

const float CPUTimer::GetTime()
{
	float total = 0.0f;

	// Iterate over all of the profiles
	ProfileMap::iterator iter;
	for (iter = profiles.begin(); iter != profiles.end(); ++iter)
	{
		total += GetTime(iter->first);
	}
	return total;
}

const float CPUTimer::GetAVGTPF(const std::string & name)
{
	_timer.Tick();


	auto i = profiles.find(name);
	if (i == profiles.end())
		return 0.0f;
	ProfileData& profile = i->second;

	if (profile.timer != nullptr)
	{

		// Get the query data
		profile._frameTime += profile.timer->TotalTimeMS();
		profile._frameCount++;


		float time = profile._ltime*1000.0f;
		if ((_timer.TotalTimeMS() - profile._timeElapsed) >= 1.0f)
		{
			profile._ltime = profile._frameTime / (float)profile._frameCount;
			profile._frameTime = 0.0f;
			profile._frameCount = 0;
			profile._timeElapsed += 1.0f;
		}

		return time;
	}
	return 0.0f;


}