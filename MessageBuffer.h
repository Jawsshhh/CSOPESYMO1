#pragma once
#include <queue>
#include <string>

class MessageBuffer {
public:
	MessageBuffer();
	void enqueueToBuffer(const std::string& message);
	std::string dequeueFromBuffer();
	std::string peek();

	bool isEmpty();
	bool getSize();
	bool clearBuffer();
private:
	std::queue<std::string> messageBuffer;
};
