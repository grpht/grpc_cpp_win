#include "ManualResetEventSlim.h"

using namespace std;
void ManualResetEventSlim::Set()
{
	lock_guard lock(_mtx);
	_signaled = true;
	_cv.notify_all();

}

void ManualResetEventSlim::Reset()
{
	lock_guard lock(_mtx);
	_signaled = false;
}

void ManualResetEventSlim::Wait()
{
	unique_lock lock(_mtx);
	_cv.wait(lock, [this] {return _signaled; });
}
