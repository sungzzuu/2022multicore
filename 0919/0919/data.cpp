#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

volatile int g_data = 0;
volatile bool g_ready = false;

void reicever()
{
	while (false == g_ready); // loop를 도는 동안 lock이 풀리지 않음
	cout << "Data = " << g_data << endl;
}

void sender()
{
	cout << "Enter Data: ";
	int a;
	cin >> a;
	g_data = a;
	g_ready = true;
}

int main()
{
	thread r{ reicever };
	thread s{ sender };

	r.join();
	s.join();
}