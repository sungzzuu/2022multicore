#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

int sum;
mutex mylock;

void thread_func()
{
	for (auto i = 0; i < 25000000; i++)
	{
		mylock.lock();
		sum += 2;
		mylock.unlock();
	}
}

int main()
{
	thread t1{ thread_func };
	thread t2{ thread_func };

	t1.join();
	t2.join();

	// 580만 근처의 값들
	cout << "Sum = " << sum << '\n';
}
