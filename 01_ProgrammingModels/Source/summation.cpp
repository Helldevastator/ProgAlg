#include <iostream>
#include <omp.h>
#include "Stopwatch.h"
#include <mutex>
#include "math.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////
// Explicit computation
static long long sum(const long long n) {
	return n*(n + 1) / 2;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Sequential summation
long long sumSerial(const int n) {
	long long sum = 0;
	for (int i = 1; i <= n; i++) {
		sum += i;
	}
	return sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Parallel summation with critical section
static long long sumPar1(const int n) {
	long long sum = 0;
	#pragma omp parallel for default(none) shared(sum)
	for (int i = 1; i <= n; i++) {
		sum += i;
	}

	return sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Parallel summation with atomic access
static long long sumPar2(const int n) {
	long long sum = 0;
	#pragma omp parallel for default(none) shared(sum) num_threads(8) schedule(guided)
	for (int i = 1; i <= n; i++) {
		#pragma omp atomic
		sum += i;
		
	}
	return sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Parallel summation with reduction
static long long sumPar3(const int n) {
	long long sum = 0;

#pragma omp parallel for default(none) reduction(+: sum) num_threads(8) schedule(guided)
	for (int i = 1; i <= n; i++) {
		sum += i;
	}
	return sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Parallel summation with explicit locks
static long long sumPar4(const int n) {
	mutex m;
	long long sum = 0;

#pragma omp parallel for default(none) shared(sum,m) num_threads(8) schedule(guided)
	for (int i = 1; i <= n; i++) {
		m.lock();
		sum += i;
		m.unlock();
	}

	return sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Different summation tests
void summation() {
	cout << "\nSummation Tests" << endl;

	const int64_t N = 50000000;
	Stopwatch sw;

	sw.Start();
	int64_t sum0 = sum(N);
	sw.Stop();
	cout << "Explicit: " << sum0 << " in " << sw.GetElapsedTimeMilliseconds() << " ms" << endl << endl;

	sw.Start();
	int64_t sumS = sumSerial(N);
	sw.Stop();
	cout << "Serial: " << sumS << " in " << sw.GetElapsedTimeMilliseconds() << " ms" << endl;
	cout << boolalpha << "The two operations produce the same results: " << (sumS == sum0) << endl << endl;

	sw.Start();
	int64_t sum1 = sumPar1(N);
	sw.Stop();
	cout << "Critical section: " << sum1 << " in " << sw.GetElapsedTimeMilliseconds() << " ms" << endl;
	cout << boolalpha << "The two operations produce the same results: " << (sum1 == sum0) << endl << endl;

	sw.Start();
	int64_t sum2 = sumPar2(N);
	sw.Stop();
	cout << "Atomic access: " << sum2 << " in " << sw.GetElapsedTimeMilliseconds() << " ms" << endl;
	cout << boolalpha << "The two operations produce the same results: " << (sum2 == sum0) << endl << endl;

	sw.Start();
	int64_t sum3 = sumPar3(N);
	sw.Stop();
	cout << "Reduction: " << sum3 << " in " << sw.GetElapsedTimeMilliseconds() << " ms" << endl;
	cout << boolalpha << "The two operations produce the same results: " << (sum3 == sum0) << endl << endl;

	sw.Start();
	int64_t sum4 = sumPar4(N);
	sw.Stop();
	cout << "Explicit locks: " << sum4 << " in " << sw.GetElapsedTimeMilliseconds() << " ms" << endl;
	cout << boolalpha << "The two operations produce the same results: " << (sum4 == sum0) << endl << endl;

}
