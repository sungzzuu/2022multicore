#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <vector>

using namespace std;
using namespace chrono;

volatile int sum = 0;

volatile int lock_mem = 0;		// 0은 아무도 lock을 얻지 않았음을 의미
								// 1은 누군가가 lock을 얻어서 critical section을 실행 중


bool CAS(volatile int *addr, int expected, int update)
{
	return atomic_compare_exchange_strong(
		reinterpret_cast<volatile atomic_int*>(addr),
		&expected, update);

}
void cas_lock()
{
	// lock_mem이 1이면 0이 될 때까지 기다린다.
	while (true)
	{
		if (lock_mem == 0) {
			if (CAS(&lock_mem, 0, 1))
				return;
			continue;
		}
	}



	// lock_mem이 0이면 atomic하게 1로 바꾸고 리턴한다.

}

void cas_unlock()
{
	// atomic하게 lock_mem을 다시 0으로 만든다.
	CAS(&lock_mem, 1, 0);

}

void worker(int num_threads)
{
	for (int i = 0; i < 50000000 / num_threads; i++)
	{
		cas_lock();
		sum += 2;
		cas_unlock();
	}
}

int main()
{
	for (int num_th = 1; num_th <= 8; num_th*=2)
	{
		sum = 0;
		auto start_t = high_resolution_clock::now();

		vector<thread> threads;

		for (int i = 0; i < num_th; ++i)
			threads.emplace_back(worker, num_th);

		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		int exec_ms = duration_cast<milliseconds>(exec_t).count();

		cout << num_th << ": Result = " << sum << " Exec Time: " << exec_ms << "ms" << endl;

	}
	}