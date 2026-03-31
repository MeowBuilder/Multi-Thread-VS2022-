#include <iostream>
#include <thread>
#include <atomic>

////volatile int x, y;
//std::atomic<int> x, y;
//const auto SIZE = 50000000;
//int trace_x[SIZE], trace_y[SIZE];
//
//void ThreadFunc0()
//{
//	for (int i = 0; i < SIZE; i++) {
//		x = i;
//		//std::atomic_thread_fence(std::memory_order_seq_cst);
//		trace_y[i] = y;
//	}
//}
//void ThreadFunc1()
//{
//	for (int i = 0; i < SIZE; i++) {
//		y = i;
//		//std::atomic_thread_fence(std::memory_order_seq_cst);
//		trace_x[i] = x;
//	}
//}
//
//int main()
//{
//	std::thread a1(ThreadFunc0);
//	std::thread a2(ThreadFunc1);
//
//	a1.join();
//	a2.join();
//
//	int count = 0;
//	for (int i = 0; i < SIZE; ++i)
//		if (trace_x[i] == trace_x[i + 1])
//			if (trace_y[trace_x[i]] == trace_y[trace_x[i] + 1]) {
//				if (trace_y[trace_x[i]] != i) continue;
//				count++;
//				std::cout << "Memory error at:" << trace_x[i] << "," << trace_y[trace_x[i]] << std::endl;
//			}
//	std::cout << "Total Memory Inconsistency: " << count << std::endl;
//}

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
	}
}

int main()
{
	std::thread a1(ThreadFunc1);
	std::thread a2(ThreadFunc2);

	a1.join();
	a2.join();
}