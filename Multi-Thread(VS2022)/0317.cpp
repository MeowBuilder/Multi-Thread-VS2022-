#include <iostream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

using namespace std::chrono;

volatile int sum;
std::mutex mylock;

//volatile bool flag[2] = { false,false };
//volatile int victim = 0;

std::atomic<bool> flag[2] = { false,false };
std::atomic<int> victim = 0;

void p_lock(int thread_id) {
	int other = 1 - thread_id;
	flag[thread_id] = true;
	victim = thread_id;
	//atomic_thread_fence(std::memory_order_seq_cst);
	while (flag[other] && victim == thread_id); // Cash thrashing
}

void p_unlock(int thread_id) {
	flag[thread_id] = false;
}

void add() {
	auto loop = 5000'0000 / 2;
	for (auto i = 0; i < loop; ++i) {
		mylock.lock();
		sum = sum + 2;
		mylock.unlock();
	}
}

void peterson_add(int thread_id) {
	auto loop = 5000'0000 / 2;
	for (auto i = 0; i < loop; ++i) {
		p_lock(thread_id);
		sum = sum + 2;
		p_unlock(thread_id);
	}
}

int main()
{
	{ // 싱글 스레드
		sum = 0;
		auto t = high_resolution_clock::now();

		for (auto i = 0; i < 5000'0000; ++i)
			sum = sum + 2;

		auto d = high_resolution_clock::now() - t;

		std::cout << std::right << std::setw(24) << "Single Thread Sum = " << std::left << std::setw(15) << sum << "\n";
		std::cout << std::right << std::setw(35) << duration_cast<milliseconds>(d).count() << " msecs\n";
	}

	{ // 멀티 스레드 (lock)
		for (auto num_threads : { 2 }) {
			sum = 0;
			std::vector<std::thread> threads;
			auto t = high_resolution_clock::now();

			for (int i = 0; i < num_threads; i++)
			{
				threads.emplace_back(add);
			}

			for (auto& t : threads) t.join();

			auto d = high_resolution_clock::now() - t;

			std::cout << num_threads << std::right << std::setw(23) << "Thread(mutax) Sum = " << std::left << std::setw(15) << sum << "\n";
			std::cout << std::right << std::setw(35) << duration_cast<milliseconds>(d).count() << " msecs\n";
		}
	}

	{ // 멀티 스레드 (lock)
		for (auto num_threads : { 2 }) {
			sum = 0;
			std::vector<std::thread> threads;
			auto t = high_resolution_clock::now();

			for (int i = 0; i < num_threads; i++)
			{
				threads.emplace_back(peterson_add, i);
			}

			for (auto& t : threads) t.join();

			auto d = high_resolution_clock::now() - t;

			std::cout << num_threads << std::right << std::setw(23) << "Thread(p_lock) Sum = " << std::left << std::setw(15) << sum << "\n";
			std::cout << std::right << std::setw(35) << duration_cast<milliseconds>(d).count() << " msecs\n";
		}
	}

}