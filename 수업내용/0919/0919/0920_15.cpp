#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
using namespace std;

const auto SIZE = 50000000;
//atomic<int> x, y;
int x, y;

int trace_x[SIZE], trace_y[SIZE];

mutex xl, yl;

void ThreadFunc0()
{
	for (int i = 0; i < SIZE; i++) {
		xl.lock();
		x = i;
		xl.unlock();
		//atomic_thread_fence(memory_order_seq_cst);
		yl.lock();
		trace_y[i] = y;
		yl.unlock();
	}
}
void ThreadFunc1()
{
	for (int i = 0; i < SIZE; i++) {
		yl.lock();
		y = i;
		yl.unlock();
		//atomic_thread_fence(memory_order_seq_cst);
		xl.lock();
		trace_x[i] = x;
		xl.unlock();
	}
}
int main()
{
	thread t1{ ThreadFunc0 };
	thread t2{ ThreadFunc1 };

	t1.join();
	t2.join();

	int count = 0;
	for (int i = 0; i < SIZE; ++i)
		if (trace_x[i] == trace_x[i + 1])
			if (trace_y[trace_x[i]] == trace_y[trace_x[i] + 1]) {
				if (trace_y[trace_x[i]] != i) continue;
				count++;
			}
	cout << "Total Memory Inconsistency: " << count << endl;
}
