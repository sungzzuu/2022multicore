#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <Windows.h>
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

class ST_NODE;
class alignas(16) ST_PTR {
public:
	ST_NODE* ptr;
	long long stamp;
public:
	ST_PTR(ST_NODE *p) : ptr(p), stamp(0) {}
	ST_PTR(ST_NODE* p, long long stamp) : ptr(p), stamp(stamp) {}

	void set(ST_NODE* p)
	{
		ptr = p;
		stamp = stamp + 1;
	}
	ST_NODE* get_ptr()
	{
		return ptr;
	}
};

class ST_NODE {
public:
	int v;
	ST_PTR next;
	volatile bool removed;
	ST_NODE() : v(-1), next(nullptr) {}
	ST_NODE(int x) : v(x), next(nullptr) {}

};

class C_QUEUE {
	NODE* head, * tail;
	mutex ll;
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
	int DEQ()
	{
		ll.lock();
		NODE* p = head;
		if (p->next == nullptr)
		{
			ll.unlock();
			return -1;
		}
		int value = p->v;
		head = head->next;
		delete p;
		ll.unlock();
		return value;
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
	NODE* volatile head, * volatile tail;
public:
	LF_QUEUE()
	{
		head = tail = new NODE{ -1 };
	}
	bool CAS(NODE*volatile* next, NODE* old_p, NODE* new_p)
	{
		return atomic_compare_exchange_strong(
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
	}
	int DEQ()
	{
		while (true) {
			NODE* first = head;
			NODE* last = tail;
			NODE* next = first->next;
			if (first != head) continue;
			if (nullptr == next) return -1;
			if (first == last) {
				CAS(&tail, last, next);
				continue;
			}
			int value = next->v;
			if (false == CAS(&head, first, next)) continue;
			delete first;
			return value;
		}

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

class ST_QUEUE {
	ST_PTR head{nullptr};
	ST_PTR tail{nullptr};
public:
	ST_QUEUE()
	{
		ST_NODE* g = new ST_NODE(-1);
		head.set(g);
		tail.set(g);
	}
	bool CAS(ST_PTR* next, ST_NODE* old_ptr, ST_NODE* new_ptr, 
		long long old_stamp, long long new_stamp)
	{
		// 리틀 엔디안
		ST_PTR old_st{old_ptr, old_stamp};
		return InterlockedCompareExchange128(
			reinterpret_cast<LONG64 volatile*>(next),
			new_stamp,
			reinterpret_cast<LONG64>(new_ptr),
			reinterpret_cast<LONG64*>(&old_st)
		);

	}
	void ENQ(int x)
	{
		// tail의 next를 읽고 tail이 바뀌지 않았나 확인하고, tail이 진짜 tail인가 확인하고 
		ST_NODE* e = new ST_NODE(x);
		while (true) {
			ST_PTR last = tail;
			ST_PTR next = last.get_ptr()->next;
			if (last.stamp != tail.stamp) continue;
			if (nullptr == next.ptr) {
				if (CAS(&last.ptr->next, nullptr, e, next.stamp, next.stamp + 1)) {
					CAS(&tail, last.get_ptr(), e, last.stamp, last.stamp + 1);
					return;
				}
			}
			else CAS(&tail, last.ptr, next.ptr, last.stamp, last.stamp + 1);
		}
	}
	int DEQ()
	{
		while (true) {
			ST_PTR first = head;
			ST_PTR last = tail;
			ST_PTR next = first.get_ptr()->next;
			if (first.stamp != head.stamp) continue;
			if (nullptr == next.ptr) return -1;
			if (first.ptr == last.ptr) {
				CAS(&tail, last.ptr, next.ptr, last.stamp, last.stamp + 1);
				continue;
			}
			int value = next.ptr->v;
			if (false == CAS(&head, first.ptr, next.ptr, first.stamp, first.stamp + 1)) continue;
			delete first.ptr;
			return value;
		}
		return 1;
	}
	void print20()
	{
		ST_PTR p = head.ptr->next;
		for (int i = 0; i < 20; ++i) {
			if (p.ptr == nullptr) break;
			cout << p.ptr->v << ", ";
			p = p.ptr->next;
		}
		cout << endl;
	}

	void clear()
	{
		while (head.get_ptr()->next.get_ptr() != nullptr) {
			ST_NODE* t = head.get_ptr();
			head.set(head.get_ptr()->next.get_ptr());
			delete t;
		}
		tail.ptr = head.ptr;
	}
};

//C_QUEUE my_queue;
//LF_QUEUE my_queue;
ST_QUEUE my_queue;
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
	for (int num_threads = 1; num_threads <= 8; num_threads *= 2) {
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