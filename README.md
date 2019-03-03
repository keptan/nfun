# nfun
header only utility library 
mostly async/multithreaded related

* Basic Type erasing future-based threadpool
* Basic Threadsafe ratelimiter 

```cpp
int main (void)
{
	RateLimiter syncro(3,1000);
	ScheduleDad syncroIO(8);
	FutureDad dad(1);

	using namespace std::chrono_literals;

	const auto task = [&]()
	{ 
		syncro.waitAndUse();
		std::cout << "task" << std::endl;
		std::this_thread::sleep_for(0.5s);
	};

	for(int i = 0; i < 10; i++)
	{
		dad.addTask([&]()
		{
			syncroIO.addTask(task);
		});
	}

	auto future = dad.addTask( [](){std::cout << "hello!" << std::endl;});

	future.get();
	std::cout << "hello task ran!" << std::endl;



	dad.join_finish();
	syncroIO.join_finish();
	
}
```
