#include "CPUTick.h"

void CPUTick::start(int batch_process_freq) {
	running = true;
	tickThread = std::thread([=] {
		while (running) {
			std::this_thread::sleep_for(std::chrono::milliseconds(batch_process_freq));
			tick++;
		}
		});
}

void CPUTick::stop() {
	running = false;
	if (tickThread.joinable()) {
		tickThread.join();
	}
}

uint64_t CPUTick::getTick() const {
	return tick.load();
}