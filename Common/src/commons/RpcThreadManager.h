#include <functional>
#include <thread>
#include <vector>
#include <unordered_map>
#include <string>
#include <any>



class RpcThreadManager
{
	using QuitCallback = std::function<void(void)>;
public:
	struct ThreadHandle
	{
		std::thread th;
	};

	static RpcThreadManager& Instance()
	{
		static RpcThreadManager instance;
		return instance;
	}
	
	std::shared_ptr<ThreadHandle> BeginThread(std::function<void(std::any)> func, std::any arg = nullptr, QuitCallback quitCallback = nullptr)
	{
		auto handle = std::make_shared<ThreadHandle>();
		handle->th = std::thread(func, arg);
		_threads.emplace(handle, quitCallback);
		return handle;
	}

	void RemoveThread(std::shared_ptr< ThreadHandle> handle)
	{
		if (!_threads.contains(handle))
			return;
		auto quitCallback = _threads[handle];
		if (quitCallback != nullptr)
			quitCallback();
		if (handle->th.joinable())
			handle->th.join();
	}

	void QuitAllAndWiatForClose()
	{
		_quit = true;

		std::unordered_map<std::shared_ptr<ThreadHandle>, QuitCallback> copy = _threads;

		for (auto& [handle, quitCallback] : copy)
		{
			RemoveThread(handle);
		}
	}

	bool IsQuit() const { return _quit; }
private:
	std::unordered_map<std::shared_ptr<ThreadHandle>, QuitCallback> _threads;
	bool _quit = false;
};