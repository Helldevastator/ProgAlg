#pragma once

#include "RWLock.h"
#include <chrono>
#include <iostream>
#include <mutex>

class BankAccount {
	mutable RWLock m_lock;	// mutable: can be modified even in const methods
	double m_balance = 0;	// bank account balance
	mutable mutex m_mutex;

public:
	void deposit(double amount) {
		m_lock.lockW();
		//pretend to calculate something
		this_thread::sleep_for(chrono::milliseconds(1000));
		m_balance += amount;
		m_lock.unlockW();
	}

	double getBalance() const {
		m_lock.lockR();
		//pretend to calculate something
		this_thread::sleep_for(chrono::milliseconds(500));
		double balanceCopy = m_balance;
		{
			unique_lock<std::mutex> monitor(m_mutex);
			cout << "thread " << this_thread::get_id() << ": balance is = " << balanceCopy << " readers " << getReaders() << endl;
		}
		
		m_lock.unlockR();

		return balanceCopy;
	}

	size_t getReaders() const {
		return m_lock.getReaders();
	}
};