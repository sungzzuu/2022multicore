#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>
using namespace std;
using namespace chrono;

atomic<int> sum;
constexpr int LOOP = 100000000;
mutex mylock;

void worker(int num_thread)
{
	for (auto i = 0; i < LOOP/ num_thread; i++)
	{
		//mylock.lock();
		sum = sum + 1;
		//mylock.unlock();
	}

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
 