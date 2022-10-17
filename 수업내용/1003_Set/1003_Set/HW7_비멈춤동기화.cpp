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

//class NODE {
//	mutex	n_lock;
//
//public:
//	int				v;
//	NODE* volatile	next;
//	volatile bool	removed; // 실행순서가 바뀌면 안되는 애라서
//
//	NODE() : v(-1), next(nullptr), removed(false) {}
//	NODE(int x) : v(x), next(nullptr), removed(false) {}
//	void lock()
//	{
//		n_lock.lock();
//	}
//	void unlock()
//	{
//		n_lock.unlock();
//	}
//};
//
//class NODE_SP {
//	mutex	n_lock;
//
//public:
//	int					v;
//	shared_ptr<NODE_SP> next;
//	volatile bool		removed; // 실행순서가 바뀌면 안되는 애라서
//
//	NODE_SP() : v(-1), next(nullptr), removed(false) {}
//	NODE_SP(int x) : v(x), next(nullptr), removed(false) {}
//	void lock()
//	{
//		n_lock.lock();
//	}
//	void unlock()
//	{
//		n_lock.unlock();
//	}
//};
class LF_NODE;
class LF_PTR {
	unsigned long long next;
public:
	LF_PTR() :next(0) {}
	LF_PTR(bool marking, LF_NODE* ptr)
	{
		next = reinterpret_cast<unsigned long long>(ptr);
		if (marking) next = next | 1;
	}
	LF_NODE* get_ptr()
	{
		return reinterpret_cast<LF_NODE*>(next & 0xfffffffffffffffe); //  // (next >> 1) << 1
	}

	bool get_removed()
	{
		return (next & 1) == 1;
	}

	LF_NODE* get_ptr_removed(bool* removed) // atomic하게 처리
	{
		unsigned long long cur_next = next;
		*removed = (cur_next & 1) == 1;
		return reinterpret_cast<LF_NODE*>(cur_next & 0xfffffffffffffffe);
	}

	bool CAS(LF_NODE* o_ptr, LF_NODE* n_ptr, bool o_mark, bool n_mark)
	{
		unsigned long long o_next = reinterpret_cast<unsigned long long>(o_ptr);
		if (o_mark) o_next++;

		unsigned long long n_next = reinterpret_cast<unsigned long long>(n_ptr);
		if (n_mark) n_next++;

		return atomic_compare_exchange_strong(reinterpret_cast<atomic_uint64_t*>(&next), &o_next, n_next);
	}
};

class LF_NODE {

public:
	int		v;
	LF_PTR	next;
	LF_NODE() : v(-1), next(false, nullptr){}
	LF_NODE(int x) : v(x), next(false, nullptr) {}
	LF_NODE(int x, LF_NODE* ptr) : v(x), next(false, ptr) {}


};

class LF_SET {
	LF_NODE head, tail;
public:
	LF_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = LF_PTR(false, &tail);
		// validate -> lock이 없어 한다고 안전하지 않음. CAS 안에서 수정 검사를 해야지
	}
	void Find(LF_NODE* &prev, LF_NODE* &curr, int x)
	{
		while (true)
		{
			retry:
			prev = &head;
			curr = prev->next.get_ptr();
			while (true)
			{
				bool removed{};
				LF_NODE* succ = curr->next.get_ptr_removed(&removed); // 값을 atomic하게 읽어야함
				
				while (!removed) {
					if (!prev->next.CAS(curr, succ, false, false))
						goto retry;

					curr = succ;
					succ = curr->next.get_ptr_removed(&removed);
				}
				
				if (curr->v >= x) return;


				prev = curr;
				curr = succ;

			}
		}
	
	}
	bool ADD(int x)
	{
		while (true)
		{
			LF_NODE* prev = &head;
			LF_NODE* curr = prev->next.get_ptr();
			//LF_NODE* prev, * curr;
			Find(prev, curr, x);

			if (curr->v != x) {
				LF_NODE* node = new LF_NODE{ x ,curr };
				if(prev->next.CAS(curr, node, false, false))
					return true;
				delete node;
			}
			else
			{
				return false;
			}
		}

	}

	bool REMOVE(int x)
	{
		//while (true)
		//{
		//	NODE* prev = &head;
		//	NODE* curr = prev->next;
		//	while (curr->v < x) {
		//		prev = curr;
		//		curr = curr->next;
		//	}
		//	prev->lock(); curr->lock();
		//	if (validate(prev, curr))
		//	{
		//		if (curr->v != x) {
		//			curr->unlock();
		//			prev->unlock();
		//			return false;
		//		}
		//		else {
		//			curr->removed = true;
		//			prev->next = curr->next;
		//			curr->unlock();
		//			prev->unlock();
		//			//delete curr;
		//			return true;
		//		}
		//	}
		//	curr->unlock();
		//	prev->unlock();
		//	continue;
		//}

		return true;

	}

	bool CONTAINS(int x)
	{
		LF_NODE* node = head.next.get_ptr();
		//while (curr->v < x) {
		//	prev = curr;
		//	curr = curr->next;
		//}
		//return curr->v == x && !curr->removed;
		return false;
	}
	void print20()
	{
		LF_NODE* p = head.next.get_ptr();
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next.get_ptr();
		}
		cout << endl;
	}

	void clear()
	{
		LF_NODE* p = head.next.get_ptr();
		while (p != &tail) {
			LF_NODE* t = p;
			p = p->next.get_ptr();
			delete t;
		}
		head.next = LF_PTR{ false, &tail };
	}
};

//SET my_set;		// 성긴동기화
//F_SET my_set;		// 세밀한동기화
//O_SET my_set;		// 낙천적인동기화
//L_SET my_set;		// 게으른동기화
//L_SET_SP my_set;	// shared_ptr
LF_SET my_set;		// 비멈춤동기화

class HISTORY {
public:
	int op; // add. remove, contain 여부
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

void worker(vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < 4000000 / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % 1000;
			my_set.ADD(v);
			break;
		}
		case 1: {
			int v = rand() % 1000;
			my_set.REMOVE(v);
			break;
		}
		case 2: {
			int v = rand() % 1000;
			my_set.CONTAINS(v);
			break;
		}
		}
	}
}

// add, remove하면서 history 저장
void worker_check(vector<HISTORY>* history, int num_threads)
{
	for (int i = 0; i < 4000000 / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % 1000;
			history->emplace_back(0, v, my_set.ADD(v));
			break;
		}
		//case 1: {
		//	int v = rand() % 1000;
		//	history->emplace_back(1, v, my_set.REMOVE(v));
		//	break;
		//}
		//case 2: {
		//	int v = rand() % 1000;
		//	history->emplace_back(2, v, my_set.CONTAINS(v));
		//	break;
		//}
		}
	}
}

// 검사하는 부분 - add, remove한 동작을 넣는다.
void check_history(array <vector <HISTORY>, 16>& history, int num_threads)
{
	array <int, 1000> survive = {};
	cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i) {
		for (auto& op : history[i]) {
			if (false == op.o_value) continue;
			if (op.op == 3) continue;
			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}
	for (int i = 0; i < 1000; ++i) {
		int val = survive[i];
		if (val < 0) {
			cout << "The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			cout << "The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (my_set.CONTAINS(i)) {
				cout << "The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == my_set.CONTAINS(i)) {
				cout << "The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	cout << " OK\n";
}

int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2) {
		vector <thread> threads;
		array<vector <HISTORY>, 16> history;
		my_set.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &history[i], num_threads);
		for (auto& th : threads)
			th.join();
		// 검사를 해서 실행속도 오류가 있음.

		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_set.print20();
		cout << num_threads << "Threads.  Exec Time : " << exec_ms << endl;
		check_history(history, num_threads);
	}

	for (int num_threads = 1; num_threads <= 16; num_threads *= 2) {
		vector <thread> threads;
		array<vector <HISTORY>, 16> history;
		my_set.clear();
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &history[i], num_threads);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_set.print20();
		cout << num_threads << "Threads.  Exec Time : " << exec_ms << endl;
		check_history(history, num_threads);
	}
}