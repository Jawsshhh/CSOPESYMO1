#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>


class CPUTick {
public:
	void start(int batch_process_freq);
	void stop();
	uint64_t getTick() const;

private:
	std::atomic_bool running = false;
	std::thread tickThread;
	std::atomic<uint64_t> tick = 0;
};