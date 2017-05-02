
#pragma once
#include "VulkanHelpers.h"
#include <map>

class GPUTimer
{
	struct ProfileData
	{
		uint8_t currentFrame;
		uint8_t currentTimeFrame;
		uint32_t* start;
		uint32_t* end;

	};

public:
	GPUTimer(VkDevice device, uint8_t queryLatency, float timestampPeriod);
	~GPUTimer();

	void Start(VkCommandBuffer& buffer, uint64_t GUID);
	void End(VkCommandBuffer& buffer, uint64_t GUID);
	const double GetTime(uint64_t GUID);
	const double GetAverageTimePerSecond(uint64_t GUID);

private:
	VkDevice _device;
	VkQueryPool _pool;
	uint32_t _maxCounters;
	uint8_t _queryLatency;
	double _freq;
	std::map<uint64_t, ProfileData> _timers;
};

