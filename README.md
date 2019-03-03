# nfun
header only utility library 
mostly async/multithreaded related

* Basic Type erasing future-based threadpool
* Basic Threadsafe ratelimiter 

```cpp
int main (void)
{
	RateLimiter syncro(3,1000); //rate limiter, 3 requests per 1 second 
	ScheduleDad syncroIO(8); //thread pool with 8 threads
	FutureDad dad(1); //thread pool with a single thread

	using namespace std::chrono_literals;

	const auto task = [&]()
	{ 
		syncro.waitAndUse();
		std::cout << "task" << std::endl;
		std::this_thread::sleep_for(0.5s);
	};


	//queue up a bunch of tasks that are rate limited and take time 
	for(int i = 0; i < 10; i++)
	{
		dad.addTask([&]()
		{
			syncroIO.addTask(task);
		});
	}

	//return a future to retrieve a value from threadpool later
	auto future = dad.addTask( []() -> int
			{std::cout << "hello!" << std::endl;
			 return 1;
			});

	const auto val = future.get();
	std::cout << "hello task ran! " << val << std::endl;

	//join threads
	dad.join_finish();
	syncroIO.join_finish();
	
}
```
