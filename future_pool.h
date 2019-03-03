#pragma once
#include <thread>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <queue>
#include <future>


class fwrap
{
	struct impl_base
	{
		virtual void call (void) = 0;
		virtual ~impl_base (void)
		{}
	};

	std::unique_ptr<impl_base> impl;
	
	template<typename F>
	struct impl_type : impl_base
	{
		F f;
		impl_type(F&& f_) : f(std::move(f_)) 
		{}

		void call () { f();}
	};

public:

	template<typename F>
	fwrap (F&& f)
		: impl( new impl_type<F>(std::move(f)))
	{}

	void operator () (void) { impl->call();}
	fwrap (void) = default;
	fwrap (fwrap&& o)
		: impl(std::move(o.impl))
	{}

	fwrap& operator= (fwrap&& o)
	{
		impl=std::move(o.impl);
		return *this;
	}

	fwrap (const fwrap&) = delete;
	fwrap (fwrap&)		 = delete;
	fwrap& operator = (const fwrap&) = delete;
};






class FutureDad 
{
	//locks, notifiers, and a flag
	//workers wait for cVar to notify them when tasks is pushed
	mutable std::mutex m; 
	mutable std::condition_variable cVar;
	std::queue<fwrap> tasks;
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

			if(tasks.empty() && quit)
			{
				lock.unlock();
				break;
			}

			auto f = std::move(tasks.front());
			tasks.pop();
			lock.unlock();

			f();
			eat();
		}
	}

public:

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


	FutureDad (const int threads = 2)
		: quit(false)
	{
		for(int i = 0; i < threads; i++)
		{
			workers.emplace_back(&FutureDad::eat, this);
		}
	}

	template<typename F>
	std::future<typename std::result_of<F()>::type> addTask (F f)
	{
		using result_type = typename std::result_of<F()>::type;

		std::packaged_task<result_type()> task (std::move(f));
		std::future res = task.get_future();
		{
		std::scoped_lock(m);
		tasks.push(std::move(task));
		}
		cVar.notify_one();

		return res;
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

	void clear (void)
	{
		std::scoped_lock(m);
		while(!tasks.empty()) tasks.pop();
	}



	~FutureDad (void)
	{
		join_finish();
	}
};
