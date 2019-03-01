#include <thread>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <queue>


template <typename F>
class scope_guard 
{
	F func;
	bool dismissed;

	public:
		explicit scope_guard (F func_)
			: func(func_), dismissed(false)
		{}

		~scope_guard (void) 
		{
			if(!dismissed)
			{
				std::cout << "thread guard running\n";
				func();
			}
		}

		void dismiss (void)
		{
			dismissed = true;
		}
};

//very simple fixed threadcount threadpool
class ScheduleDad 
{
	//locks, notifiers, and a flag
	//workers wait for cVar to notify them when tasks is pushed
	mutable std::mutex m; 
	mutable std::condition_variable cVar;
	std::queue<std::function<void()>> tasks;
	bool quit;

	//two workers, could be a vector of threads with std::hardware_count or whatever
	std::vector<std::thread> workers;
	
	//waiting loop
	void eat (void)
	{
		while(true)
		{
			std::unique_lock lock(m);
			cVar.wait( lock, [&](){return !tasks.empty() || quit;});

			if(tasks.empty() && quit) return;

			const auto f = tasks.front();
			tasks.pop();
			lock.unlock();

			f();
			eat();
		}
	}

	//finishes all tasks by default
	void join_finish (void)
	{
		{
		std::scoped_lock(m);
		quit = true;
		}
		cVar.notify_all();

		for(auto& t: workers) if(t.joinable()) t.join();
	}

public:

	ScheduleDad (const int threads = 2)
		: quit(false)
	{
		for(int i = 0; i < threads; i++)
		{
			workers.emplace_back(&ScheduleDad::eat, this);
		}
	}

	void addTask (std::function<void()> f)
	{
		{
		std::scoped_lock(m);
		tasks.push(f);
		}
		cVar.notify_one();
	}

	//finishes current tasks and then dumps the queue
	void join_abort (void)
	{
		{
		std::scoped_lock(m);
		quit = true;
		while(!tasks.empty()) tasks.pop();
		}

		cVar.notify_all();

		for(auto& t: workers) if(t.joinable()) t.join();
	}

	~ScheduleDad (void)
	{
		join_finish();
	}
};
