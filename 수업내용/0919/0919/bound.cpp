#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
using namespace std;

volatile bool done = false;
volatile int* bound;
int error;
void ThreadFunc1()
{
	for (int j = 0; j <= 25000000; ++j) *bound = -(1 + *bound);
	done = true;
}
void ThreadFunc2()
{
	while (!done) {
		int v = *bound;
		if ((v != 0) && (v != -1)) error++;
		cout << hex << v << ", ";

	}
}

int main()
{
	/*int a = 0;
	bound = &a;*/
	
	// 메모리 주소를 64의 배수로 만들고 2를뺀다.
	// 캐시
	int arr[32];
	long long addr = reinterpret_cast<long long>(&arr[30]);
	addr = (addr / 64) * 64;
	addr -= 1;
	bound = reinterpret_cast<int*>(addr);
	*bound = 0;

	thread t1{ ThreadFunc1 };
	thread t2{ ThreadFunc2 };

	t1.join();
	t2.join();

	cout << "Total Memory Inconsistency: " << error << endl;
}