
class RateLimiter
{
	using Time = std::chrono::time_point< std::chrono::high_resolution_clock>;
	using Duration = std::chrono::duration<int, std::milli>;

	const Duration duration;
	const Duration per;

	std::queue<Time> times;
	const int limit;

	Time marked;

	std::mutex mutex;

public:

	RateLimiter (int l, int d) 
		: duration(d), per(d / l), limit(l), marked(std::chrono::system_clock::now())
	{}

	void waitAndUse (void)
	{
		std::scoped_lock lk (mutex);
		const auto now = std::chrono::high_resolution_clock::now();

		while(!times.empty())
		{
			if( now > times.front() + duration)
			{
				times.pop();
				continue;
			}
			break;
		}

		if(times.size() < limit)
		{
			times.push( std::chrono::high_resolution_clock::now());
			return;
		}

		std::this_thread::sleep_for( (times.front() + duration - now));
		return;
	}

};
