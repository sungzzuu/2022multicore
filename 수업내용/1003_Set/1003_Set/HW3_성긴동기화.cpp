#include <limits>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;
using namespace chrono;

class NODE {
public:
	int v;
	NODE* next;
	mutex myl;
	NODE() : v(-1), next(nullptr) {}
	NODE(int x) : v(x), next(nullptr) {}

};


class SET {
	NODE head, tail;
public:
	SET()
		:head(0x80000000), tail(0x7fffffff)
	{
		head.next = &tail;
		tail.next = nullptr;
	}
	bool ADD(int x)
	{
		NODE* prev = &head;
		NODE* curr = prev->next;
		while (curr->v < x)
		{
			prev = curr;
			curr = curr->next;
		}
		if (curr->v != x) {

			NODE* node = new NODE{ x };
			node->next = curr;
			prev->next = node;
			my_l.unlock();

			return true;
		}
		my_l.unlock();
		return false;

	}

	bool REMOVE(int x)
	{
		NODE* prev = &head;
		my_l.lock();
		NODE* curr = prev->next;
		while (curr->v < x)
		{
			prev = curr;
			curr = curr->next;
		}
		if (curr->v == x) {
			prev->next = curr->next;
			delete curr;
			my_l.unlock();

			return true;
		}
		my_l.unlock();
		return false;
	}

	bool CONTAINS(int x)
	{
		NODE* prev = &head;
		my_l.lock();
		NODE* curr = prev->next;
		while (curr->v < x)
		{
			prev = curr;
			curr = curr->next;
		}
		if (curr->v == x) {
			my_l.unlock();
			return true;
		}
		my_l.unlock();
		return false;
	}

	void print()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i)
		{
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << '\n';
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail)
		{
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

SET my_set;

void worker(int num_th)
{
	for (int i = 0; i < 4000000 / num_th; ++i)
	{
		int op = rand() % 3;
		switch (op)
		{
		case 0:
			my_set.ADD(rand() % 1000);
			break;
		case 1:
			my_set.REMOVE(rand() % 1000);
			break;
		case 2:
			my_set.CONTAINS(rand() % 1000);
			break;
		default:
			break;
		}
	}
}
int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		vector<thread> threads;
		my_set.clear();
		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
		{
			threads.emplace_back(worker, num_threads);
		}
		for (auto& th : threads)
			th.join();

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		int exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_set.print();
		cout << num_threads << "Threads,  EXEC_MS: " << exec_ms << "ms\n";
	}

}