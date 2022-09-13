#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>
using namespace std;
using namespace chrono;

volatile int sum[8*8];
constexpr int LOOP = 50000000;
mutex mylock;

void worker(int thread_id, int num_thread)
{
	for (auto i = 0; i < LOOP / num_thread; i++)
		sum[thread_id*8] += 2;
}

int main()
{
	for (int num_thread = 1; num_thread <= 8; num_thread*=2)
	{
		for (auto& a : sum) a = 0;

		vector<thread> threads;
		auto t = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
		{
			threads.emplace_back(worker, i, num_thread);
		}
		for (auto& th : threads)
			th.join();
		auto d = high_resolution_clock::now() - t;
		cout << duration_cast<milliseconds>(d).count() << " msecs ";
		int f_sum = 0;
		for (auto& a : sum) f_sum += a;

		cout << "Threads "<< num_thread <<" Sum = " << f_sum << '\n';

	}
}
 