#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>
using namespace std;
using namespace chrono;

int sum;
constexpr int LOOP = 50000000;
mutex mylock;

void worker(int num_thread)
{
	volatile int local_sum = 0;
	for (auto i = 0; i < LOOP/ num_thread; i++)
	{
		//mylock.lock();
		//sum = sum + 1; // DATA RACE 일어남.
		//sum += 1; // data race 일어나지 않음.
		local_sum += 2;
		//mylock.unlock();
	}
	mylock.lock();
	sum += local_sum;
	mylock.unlock();
}

int main()
{

	for (int num_thread = 1; num_thread <= 8; num_thread*=2)
	{
		sum = 0;
		vector<thread> threads;
		auto t = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
		{
			threads.emplace_back(worker, num_thread);
		}
		for (auto& th : threads)
			th.join();
		auto d = high_resolution_clock::now() - t;
		cout << duration_cast<milliseconds>(d).count() << " msecs ";
		// 580만 근처의 값들
		cout << "Threads "<< num_thread <<" Sum = " << sum << '\n';

	}



}
 