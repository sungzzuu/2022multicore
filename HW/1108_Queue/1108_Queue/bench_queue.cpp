#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;
using namespace chrono;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};

class NODE {
public:
	int v;
	NODE* volatile next;
	NODE() : v(-1), next(nullptr) {}
	NODE(int x) : v(x), next(nullptr) {}
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
		NODE* e = new NODE{ x };
		ll.lock();
		tail->next = e;
		tail = e;
		ll.unlock();
	}

	int DEQ()
	{
		ll.lock();
		if (head->next == nullptr) {
			ll.unlock();
			return -1;
		}
		int res = head->next->v;
		NODE* t = head;
		head = head->next;
		ll.unlock();
		delete t;
		return res;
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
		while (nullptr != head->next) {
			NODE* t = head;
			head = head->next;
			delete t;
		}
		tail = head;
	}
};

class LF_QUEUE {
	NODE* volatile head;
	NODE* volatile tail;
public:
	LF_QUEUE()
	{
		head = tail = new NODE{ -1 };
	}
	bool CAS(NODE* volatile * next, NODE* old_p, NODE* new_p)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<volatile atomic_llong *>(next), 
			reinterpret_cast<long long *>(&old_p),
			reinterpret_cast<long long>(new_p));
	}
	void ENQ(int x)
	{
		NODE* e = new NODE{ x };
		while (true) {
			NODE* last = tail;
			NODE* next = last->next;
			if (last != tail) continue;
			if (nullptr == next) {
				if (true == CAS(&last->next, nullptr, e)) {
					CAS(&tail, last, e);
					return;
				}
			}
			else
				CAS(&tail, last, next);
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
			if (false == CAS(&head, first, next))
				continue;
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
		while (nullptr != head->next) {
			NODE* t = head;
			head = head->next;
			delete t;
		}
		tail = head;
	}
};

class SH_NODE {
public:
	int v;
	atomic<shared_ptr<SH_NODE>> next;
	SH_NODE() : v(-1) {}
	SH_NODE(int x) : v(x) {}
};

class LF_SH_QUEUE {
	atomic<shared_ptr<SH_NODE>> head;
	atomic<shared_ptr<SH_NODE>> tail;
public:
	LF_SH_QUEUE()
	{
		head = make_shared<SH_NODE>(-1);
		tail.store(head);
	}
	bool CAS(atomic<shared_ptr<SH_NODE>>& next, shared_ptr<SH_NODE> old_p, const shared_ptr<SH_NODE>& new_p)
	{
		return next.compare_exchange_strong(old_p, new_p);
	}
	void ENQ(int x)
	{
		shared_ptr<SH_NODE> e = make_shared<SH_NODE>(x);
		while (true) {
			shared_ptr<SH_NODE> last = tail;
			shared_ptr<SH_NODE> next = last->next;
			shared_ptr<SH_NODE> comp = tail;
			if (last != comp) continue;
			if (nullptr == next) {
				if (true == CAS(last->next, nullptr, e)) {
					CAS(tail, last, e);
					return;
				}
			}
			else
				CAS(tail, last, next);
		}
	}

	int DEQ()
	{
		while (true) {
			shared_ptr<SH_NODE>  first = head;
			shared_ptr<SH_NODE>  last = tail;
			shared_ptr<SH_NODE>  next = first->next;
			shared_ptr<SH_NODE> comp = head;
			if (first != comp) continue;
			if (nullptr == next) return -1;
			if (first == last) {
				CAS(tail, last, next);
				continue;
			}
			int value = next->v;
			if (false == CAS(head, first, next))
				continue;
			return value;
		}
	}

	void print20()
	{
		shared_ptr<SH_NODE> p = head;
		p = p->next;
		for (int i = 0; i < 20; ++i) {
			if (p == nullptr) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head.store(tail);
	}
};

//C_QUEUE my_queue;
//LF_QUEUE my_queue;
LF_SH_QUEUE my_queue;
void worker(int num_threads)
{
	for (int i = 0; i < 10000000 / num_threads; ++i) 
		if ((rand() % 2 == 0) || (i < 32 / num_threads))
			my_queue.ENQ(i);
		else
			my_queue.DEQ();
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
}