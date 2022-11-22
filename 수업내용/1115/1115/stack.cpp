#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <Windows.h>
using namespace std;
using namespace chrono;
constexpr int NUM_TEST = 10000000;
constexpr int MAX_THREAD = 16;
constexpr int TIME_OUT = 1000;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int v;
	NODE* volatile next;
	NODE() : v(-1), next(nullptr){}
	NODE(int x) : v(x), next(nullptr){}
};

class C_STACK {
	NODE* top;
	mutex ll;
public:
	C_STACK()
	{
		top = new NODE{ -1 };
	}

	void Push(int x)
	{
		NODE* p = new NODE(x);
		ll.lock();
		p->next = top;
		top = p;
		ll.unlock();
	}
	int Pop()
	{
		ll.lock();
		if (top == nullptr)
		{
			ll.unlock();
			return -2;
		}

		int value = top->v;
		NODE* p = top;
		top = top->next;
		ll.unlock();

		delete p;
		return value;
	}
	void print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		while (top != nullptr) {
			NODE* t = top;
			top = top->next;
			delete t;
		}
	}
};
class LF_STACK {
	NODE* volatile top;
public:
	LF_STACK()
	{
		top = new NODE{ -1 };
	}
	bool CAS(NODE* volatile* next, NODE* old_p, NODE* new_p)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_long*>(next),
			reinterpret_cast<long*>(&old_p),
			reinterpret_cast<long>(new_p));
	}
	void Push(int x)
	{
		NODE* p = new NODE(x);
		p->next = top;
		while (true) {
			NODE* last = top;
			if (CAS(&top, last, p)) {
				return;
			}
		}
	}
	int Pop()
	{
		if (top == nullptr)
		{
			return -2;
		}
		while (true)
		{
			NODE* last = top;
			NODE* next = last->next;
			if (CAS(&top, last, next))
			{
				int value = last->v;
				return value;
			}
		}
	}
	void print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		while (top != nullptr) {
			NODE* t = top;
			top = top->next;
			delete t;
		}
	}
};

//class BACKOFF {
//	int limit;
//
//public: 
//	BACKOFF() : limit(10) {}
//	void backoff()
//	{
//		int delay = rand() % limit + 1; // delay가 0이면 int의 가장 큰 값만큼 딜레이하게 된다.
//		limit = limit * 2;
//		if (limit > 200) limit = 200;
//		_asm mov eax, delay;
//	bo_loop:
//		_asm dec eax;
//		_asm jnz bo_loop;
//	}
//};
//class LF_BO_STACK {
//	NODE* volatile top;
//public:
//	LF_BO_STACK()
//	{
//		top = new NODE{ -1 };
//	}
//	bool CAS(NODE* volatile* next, NODE* old_p, NODE* new_p)
//	{
//		return atomic_compare_exchange_strong(
//			reinterpret_cast<volatile atomic_long*>(next),
//			reinterpret_cast<long*>(&old_p),
//			reinterpret_cast<long>(new_p));
//	}
//	void Push(int x)
//	{
//		NODE* p = new NODE(x);
//		BACKOFF bo;
//		p->next = top;
//		while (true) {
//			NODE* last = top;
//			p->next = last;
//			if (CAS(&top, last, p)) {
//				return;
//			bo.backoff();
//			}
//		}
//	}
//	int Pop()
//	{
//		BACKOFF bo;
//
//		if (top == nullptr)
//		{
//			return -2;
//		}
//		while (true)
//		{
//			NODE* last = top;
//			NODE* next = last->next;
//			if (CAS(&top, last, next))
//			{
//				int value = last->v;
//				return value;
//			}
//			bo.backoff();
//
//		}
//	}
//	void print20()
//	{
//		NODE* p = top;
//		for (int i = 0; i < 20; ++i) {
//			if (p == nullptr) break;
//			cout << p->v << ", ";
//			p = p->next;
//		}
//		cout << endl;
//	}
//
//	void clear()
//	{
//		while (top != nullptr) {
//			NODE* t = top;
//			top = top->next;
//			delete t;
//		}
//	}
//};
constexpr int EMPTY = 00;
constexpr int WAITING = 01;
constexpr int BUSY = 10;

class EXCHANGER {
	atomic_int value;	// MSB 2 bit	00:EMPTY	01:WAITING		10:BUSY
public:
	EXCHANGER() : value(0) {}
	int exchanger(int x, bool* is_busy)
	{
		for(int j = 0; j < TIME_OUT; ++j)
		{
			int cur_value = value;
			int state = value >> 30;
			switch (state)
			{
			case EMPTY: // EMPTY
			{
				int new_value = (WAITING << 30) | x;
				if (atomic_compare_exchange_strong(&value, &cur_value, new_value))
				{
					bool success = false;
					for (int i = 0; i < TIME_OUT; ++i)
						if (BUSY == (value >> 30))
						{
							success = true;
							break;
						}
					if (success) {
						int ret = value & 0x3fffffff;
						value = 0;
						return ret;
					}
					else {
						if (atomic_compare_exchange_strong(&value, &new_value, 0))
							return -1; // 최후까지 다른 스레드가 건들지 않음. 혼자 나감
						else { // 다른 스레드가 와서 value를 바꿨다.
							int ret = value & 0x3fffffff;
							value = 0;
							return ret;
						}

					}
				}
				else continue;
			}
			break;
			case WAITING:
			{
				int new_value = (BUSY << 30) | x;
				if (atomic_compare_exchange_strong(&value, &cur_value, new_value))
				{
					return cur_value & 0x3fffffff;
				}
				else continue;
			}
			break;
			case BUSY:
			{
				*is_busy = true;
				continue;
			}
			break;
			}
		}
		*is_busy = true;
		return -2;
	}

};
class EL_ARRAY {
	atomic_int range;
	EXCHANGER ex_array[MAX_THREAD];

public:
	EL_ARRAY() : range(1) {}
	int visit(int x)
	{
		int slot = rand() % range;
		bool busy = false;
		int ret = ex_array[slot].exchanger(x, &busy);

		int old_range = range;
		if ((- 2 == ret) && (range > 1)) // 타임아웃
			atomic_compare_exchange_strong(&range, &old_range, old_range - 1);
		if (busy && (range < MAX_THREAD / 2)) // 충돌이 심함
			atomic_compare_exchange_strong(&range, &old_range, old_range + 1);

		return ret;
	}
};

class LF_EL_STACK {
	NODE* volatile top;
	EL_ARRAY _el;

public:
	LF_EL_STACK() :top(nullptr)
	{
		
	}
	bool CAS(NODE* volatile* next, NODE* old_p, NODE* new_p)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_long*>(next),
			reinterpret_cast<long*>(&old_p),
			reinterpret_cast<long>(new_p));
	}
	void Push(int x)
	{
		NODE* p = new NODE(x);
		p->next = top;
		while (true) {
			NODE* last = top;
			p->next = last;
			if (CAS(&top, last, p)) {
				return;
				int ret = _el.visit(x);
				if (-1 == ret) return; // 성공
				//else if (-2 == ret) continue; // 타임아웃
				//else if(0 < ret) continue; // 실패
			}
		}
	}
	int Pop()
	{
	/*	BACKOFF bo;

		if (top == nullptr)
		{
			return -2;
		}
		while (true)
		{
			NODE* last = top;
			NODE* next = last->next;
			if (CAS(&top, last, next))
			{
				int value = last->v;
				return value;
			}
			bo.backoff();

		}*/
		return 1;
	}
	void print20()
	{
		NODE* p = top;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		while (top != nullptr) {
			NODE* t = top;
			top = top->next;
			delete t;
		}
	}
};

//C_STACK my_stack;
//LF_STACK my_stack;
//LF_BO_STACK my_stack;
LF_EL_STACK my_stack; // pop 하기 문제 해결하기

void worker(int num_threads)
{
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		if ((rand() % 2) || i < 1000 / num_threads) {
			my_stack.Push(i);
		}
		else {
			my_stack.Pop();
		}
	}

}

int main()
{
	for (int num_threads = 1; num_threads <= MAX_THREAD; num_threads *= 2) {
		vector <thread> threads;
		my_stack.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, num_threads);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_stack.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
	}

	system("pause");
}