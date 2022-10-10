#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
constexpr int MAX_THREAD = 32;
constexpr int NUM = 10000000;

using namespace std;
using namespace chrono;
class Bakery;

volatile int sum = 0;

mutex myl;
Bakery* bakery;

class Bakery
{
	atomic<bool>* flag;
	atomic<int>* label;
	int thread_num;

public:
	Bakery(int th_num)
	{
		thread_num = th_num;
		flag = new atomic<bool>[thread_num];
		label = new atomic<int>[thread_num];
		for (int i = 0; i < thread_num; ++i)
		{
			flag[i] = false;
			label[i] = 0;
		}                                                                         
	}
	bool IsMyTurn(int src, int target)
	{
		if (label[src] < label[target]) return true;
		if (label[src] == label[target])
			if (src < target) return true;

		return false;
	}
	void lock(int th_id)
	{
		flag[th_id] = true;
		int max = 0;
		for (int i = 0; i < thread_num; i++)
		{
			if (label[i] > max)
				max = label[i];
		}

		label[th_id] = max + 1;
		flag[th_id] = false;
		
		for (int i = 0; i < thread_num; i++)
		{
			if (i != th_id)
			{
				while (flag[i]);
				while (label[i] && IsMyTurn(i, th_id));
			}
		}
	}

	void unlock(int th_id)
	{
		flag[th_id] = false;
		label[th_id] = 0;
	}

};

void worker(int th_id, int th_num)
{
	for (int i = 0; i < NUM / th_num; i++)
	{
		bakery->lock(th_id);
		//myl.lock();
		sum = sum + 1;
		//myl.unlock();
		bakery->unlock(th_id);
	}
}

int main()
{
	for (int thread_num = 1; thread_num <= MAX_THREAD; thread_num *= 2)
	{
		sum = 0;
		vector<thread> threads;
		bakery = new Bakery(thread_num);
		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < thread_num; i++) threads.emplace_back(worker, i, thread_num);
		for (int i = 0; i < thread_num; i++) threads[i].join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		int exec_ms = duration_cast<milliseconds>(exec_t).count();

		cout << thread_num << ": Result = " << sum << " Exec Time: " << exec_ms << "ms" << endl;
		delete bakery;
	}
	
}