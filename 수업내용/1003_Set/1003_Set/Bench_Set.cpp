#include <limits>
#include <iostream>
#include <chrono>
#include <thread>
using namespace std;
using namespace chrono;

class NODE {
public:
	int v;
	NODE* next;
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
			return true;
		}
		return false;

	}

	bool REMOVE(int x)
	{
		NODE* prev = &head;
		NODE* curr = prev->next;
		while (curr->v < x)
		{
			prev = curr;
			curr = curr->next;
		}
		if (curr->v == x) {
			prev->next = curr->next;
			delete curr;
			return true;
		}
		return false;
	}

	bool CONTAINS(int x)
	{
		NODE* prev = &head;
		NODE* curr = prev->next;
		if (curr->v == x) {
			return true;
		}
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
	}
};


int main()
{
	SET my_set;
	auto start_t = high_resolution_clock::now();
	for (int i = 0; i < 50000000; ++i)
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

	auto end_t = high_resolution_clock::now();
	auto exec_t = end_t - start_t;
	int exec_ms = duration_cast<milliseconds>(exec_t).count();
	my_set.print();
	cout << "EXEC_MS: " << exec_ms << "ms\n";
}