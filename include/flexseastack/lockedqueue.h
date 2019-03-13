#include <queue>
#include <mutex>

// How to write a template class?

class LockedQueue
{

public:
    LockedQueue();
    ~LockedQueue();
    void lock() {bufferLock.lock();}
	void unlock() {bufferLock.unlock();}
	void push() {
		bufferLock.lock();
		lockedBuffer.push();
		bufferLock.unlock();
	}
	pop() {
		bufferLock.lock();
		auto data = lockedBuffer.push();
		bufferLock.unlock();
		return data;
	}



private:
	//Variables & Objects:
    std::mutex bufferLock;
	std::queue<Message> lockedBuffer;
};