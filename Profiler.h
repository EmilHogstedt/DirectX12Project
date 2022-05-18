#pragma once

struct ProfilerData
{
	std::string ContextName;
	double Duration;
};

template<typename LambdaFunction>
class Profiler
{
public:
	Profiler(const std::string& contextName, const LambdaFunction&& func) noexcept
		: m_ContextName{ contextName },
		  m_FunctionToCall{ func },
		  m_StartTime{ std::chrono::high_resolution_clock::now() }
	{

	}
	~Profiler()
	{
		auto end = std::chrono::high_resolution_clock::now();
		auto dif = std::chrono::duration_cast<std::chrono::microseconds>(end - m_StartTime);
		auto ms = static_cast<double>(dif.count());
		ms *= 0.001;

		ProfilerData profilerData;
		profilerData.ContextName = m_ContextName;
		profilerData.Duration = ms;

		m_FunctionToCall(profilerData);
	}
private:
	std::string m_ContextName;
	LambdaFunction m_FunctionToCall;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};

struct ProfilerManager
{
	static std::vector<ProfilerData> ProfilerDatas;
	static void Clear() noexcept
	{
		if (!ProfilerDatas.empty())
		{
			double averageDuration{ 0.0f };
			for (uint32_t i{ 0u }; i < ProfilerDatas.size(); ++i)
			{
				averageDuration += ProfilerDatas[i].Duration;
			}
			averageDuration /= ProfilerDatas.size();
			std::cout << std::setprecision(5) << averageDuration << " ms\n";
			ProfilerDatas.clear();
		}
	}
};