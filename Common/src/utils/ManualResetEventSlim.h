#pragma once
#include <condition_variable>
#include <mutex>

class ManualResetEventSlim
{
public:
	ManualResetEventSlim(bool initial = false)
		:_signaled(initial) 
	{}

	void Set();
	void Reset();
	void Wait();

private:
	std::mutex _mtx;
	std::condition_variable _cv;
	bool _signaled;
};