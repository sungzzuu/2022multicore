#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

int g_data = 0;
bool g_ready = false;

mutex l;

void reicever()
{
	l.lock();
	while (false == g_ready) // loop를 도는 동안 lock이 풀리지 않음
	{
		l.unlock();
		l.lock();
	}
	l.unlock();
	l.lock();
	cout << "Data = " << g_data << endl;
	l.unlock();

}

void sender()
{
	cout << "Enter Data: ";
	l.lock();

	cin >> g_data;
	l.unlock();

	l.lock();

	g_ready = true;
	l.unlock();

}

int main()
{
	thread r{ reicever };
	thread s{ sender };

	r.join();
	s.join();
}