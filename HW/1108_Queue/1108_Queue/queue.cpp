#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <array>
#include <mutex>

using namespace std;
using namespace chrono;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};

class NODE {
	mutex n_lock;
public:
	int v;
	NODE* volatile next;
	volatile bool removed;
	NODE() : v(-1), next(nullptr), removed(false) {}
	NODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class C_QUEUE {
	NODE *head, *tail;
	null_mutex ll;
public:
	C_QUEUE()
	{
		head = tail = new NODE{ -1 };
	}

	void ENQ(int x)
	{
		ll.lock();
		NODE* p = new NODE(x);
		tail->next = p;
		tail = p;
		ll.unlock();
	}
	void DEQ()
	{
		ll.lock();
		NODE* p = head;
		if (p->next == nullptr)
		{
			ll.unlock();
			return;
		}
		head = head->next;
		delete p;
		ll.unlock();
		return;
	}
	void print20()
	{
		NODE* p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		while (head->next != nullptr) {
			NODE* t = head;
			head = head->next;
			delete t;
		}
		tail = head;
	}
};
class LF_QUEUE {
	NODE* head, * tail;
public:
	LF_QUEUE()
	{
		head = tail = new NODE{ -1 };
	}
	bool CAS(NODE*volatile* next, NODE* old_p, NODE* new_p)
	{
		atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong*>(next),
			reinterpret_cast<long long*>(&old_p),
			reinterpret_cast<long long>(new_p));
	}
	void ENQ(int x)
	{
		NODE* p = new NODE(x);
		while (true) {
			NODE* last = tail;
			NODE* next = last->next;
			if (last != tail) continue;
			if (nullptr == next) {
				if (CAS(&(last->next), nullptr, p)) {
					CAS(&tail, last, p);
					return;
				}
			}
			else CAS(&tail, last, next);
		}
		tail->next = p;
		tail = p;
	}
	int DEQ()
	{
		/*NODE* p = head;
		if (p->next == nullptr)
		{
			return -1;
		}
		int res = p->next->v;
		head = head->next;
		delete p;
		return res;*/
	}
	void print20()
	{
		NODE* p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		while (head->next != nullptr) {
			NODE* t = head;
			head = head->next;
			delete t;
		}
		tail = head;
	}
};

C_QUEUE my_queue;

void worker(int num_threads)
{
	for (int i = 0; i < 10000000 / num_threads; ++i) {
		if ((rand() % 2) || i < 32 / num_threads) {
			my_queue.ENQ(i);
		}
		else {
			my_queue.DEQ();
		}
	}

}

int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2) {
		vector <thread> threads;
		my_queue.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, num_threads);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_queue.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
	}

	system("pause");
}