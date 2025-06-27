#include "MessageBuffer.h"

MessageBuffer::MessageBuffer() {

}

void MessageBuffer::enqueueToBuffer(const std::string& message) {
	messageBuffer.push(message);
}

std::string MessageBuffer::dequeueFromBuffer() {
	std::string front = messageBuffer.front();
	messageBuffer.pop();

	return front;
}

std::string MessageBuffer::peek() {
	return messageBuffer.front();
}

bool MessageBuffer::isEmpty()
{
	return messageBuffer.empty();
}

bool MessageBuffer::getSize()
{
	return messageBuffer.size();
}

bool MessageBuffer::clearBuffer()
{
	return false;
}