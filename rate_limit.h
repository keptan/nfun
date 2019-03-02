#include <thread>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <chrono>
#include <queue>


class RateLimiter
{
	using Time = std::chrono::time_point< std::chrono::high_resolution_clock>;
	using Duration = std::chrono::duration<int, std::milli>;

	const Duration duration;
	const Duration per;

	std::queue<Time> times;
	const int limit;

	std::mutex mutex;

public:

	RateLimiter (int l, int d) 
		: duration(d), per(d / l), limit(l)
	{}

	void waitAndUse (void)
	{
		std::scoped_lock lk (mutex);
		const auto now = std::chrono::high_resolution_clock::now();

		while(!times.empty())
		{
			const auto passed = std::chrono::duration_cast<std::chrono::milliseconds>(now - times.front());
			if(passed >= duration)
			{
				times.pop();
				continue;
			}
			break;
		}

		
		if(times.size() < limit)
		{
			times.push(now);
			return;
		}


		const auto passed = std::chrono::duration_cast<std::chrono::milliseconds>(now - times.front());
		const auto wait =   std::chrono::duration_cast<std::chrono::milliseconds>( duration - passed);
		std::this_thread::sleep_for(wait);
		times.push(std::chrono::high_resolution_clock::now());
		return;

	}

};
