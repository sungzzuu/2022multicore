#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

using namespace std;
using namespace chrono;

int sum = 0;
atomic<int> victim = 0;
atomic<bool> flag[2];

mutex l;
void p_unlock(int _id)
{
	flag[_id] = false;
}

void p_lock(int _id)
{
	int o = 1 - _id;

	flag[_id] = true;
	victim = _id;
	//atomic_thread_fence(memory_order_seq_cst);

	while (flag[o] && victim == _id)
	{

	}
}

void worker(int th_id)
{
	for (int i = 0; i < 25000000; i++)
	{
		p_lock(th_id);
		//l.lock();
		sum += 2;
		//l.unlock();
		p_unlock(th_id);
	}
}

int main()
{
	auto start_t = high_resolution_clock::now();
	thread t1{ worker, 0 };
	thread t2{ worker, 1 };
	t1.join();
	t2.join();
	auto end_t = high_resolution_clock::now();
	auto exec_t = end_t - start_t;
	int exec_ms = duration_cast<milliseconds>(exec_t).count();

	cout << "Result = " << sum << " Exec Time: " << exec_ms << "ms" << endl;
}