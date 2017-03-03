#pragma once

#include <mutex>
#include <iostream>
using namespace std;

class RWLock {
	mutex m_mutex;				// re-entrance not allowed
	condition_variable m_readingAllowed, m_writingAllowed;
	bool m_writeLocked = false;	// locked for writing
	size_t m_readLocked = 0;	// number of concurrent readers

public:
	size_t getReaders() const {
		return m_readLocked;
	}

	void lockR() {
		{
			unique_lock<std::mutex> monitor(m_mutex);
			m_readingAllowed.wait(monitor, [this]() {return !m_writeLocked;});
			m_readLocked++;
		}
	}

	void unlockR() {
		{
			unique_lock<std::mutex> monitor(m_mutex);
			//technically not needed, but if this is not the case then somebody could decrease m_readLocked while it is currently locked for writing
			m_readingAllowed.wait(monitor, [this]() {return !m_writeLocked;});
			m_readLocked--;
			if (m_readLocked == 0) {
				m_writingAllowed.notify_one();
			}
			
		}
	}

	void lockW() {
		{
			unique_lock<std::mutex> monitor(m_mutex);
			m_writingAllowed.wait(monitor, [this]() {return this->m_readLocked == 0 && !this->m_writeLocked;});
			m_writeLocked = true;
		}
	}

	void unlockW() {
		{
			unique_lock<std::mutex> monitor(m_mutex);
			m_writeLocked = false;
			m_writingAllowed.notify_one();
			m_readingAllowed.notify_all();
		}
	}
};