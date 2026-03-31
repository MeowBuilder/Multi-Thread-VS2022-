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

std::atomic<int> atomic_sum;

constexpr int MAX_THREADS = 8;
volatile int array_sum[MAX_THREADS];

void lock_add(int num_threads) {
	auto loop = 5000'0000 / num_threads;
	for (auto i = 0; i < loop; ++i) {
		mylock.lock();
		sum = sum + 2;
		mylock.unlock();
	}
}

void atomic_add(int num_threads) {
	auto loop = 5000'0000 / num_threads;
	for (auto i = 0; i < loop; ++i) {
		atomic_sum += 2;
	}
}

void optimal_add(int num_threads) {
	auto loop = 5000'0000 / num_threads;
	volatile int local_sum = 0;
	for (auto i = 0; i < loop; ++i) {
		local_sum = local_sum + 2;
	}
	mylock.lock();
	sum = sum + local_sum;
	mylock.unlock();
}


void array_add(int num_threads, int thread_id) {
	auto loop = 5000'0000 / num_threads;
	for (auto i = 0; i < loop; ++i) {
		array_sum[thread_id] = array_sum[thread_id] + 2;
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
		for (auto num_threads : { 1,2,4,6,8 }) {
		sum = 0;
		std::vector<std::thread> threads;
		auto t = high_resolution_clock::now();

		for (int i = 0; i < num_threads; i++)
		{
			threads.emplace_back(lock_add, num_threads);
		}

		for (auto& t : threads) t.join();

		auto d = high_resolution_clock::now() - t;

		std::cout << num_threads << std::right << std::setw(23) << "Thread(lock) Sum = " << std::left << std::setw(15) << sum << "\n";
		std::cout << std::right << std::setw(35) << duration_cast<milliseconds>(d).count() << " msecs\n";
		}
	}

	{ // 멀티 스레드 (atomic)
		for (auto num_threads : { 1,2,4,6,8 }) {
			atomic_sum = 0;
			std::vector<std::thread> threads;
			auto t = high_resolution_clock::now();

			for (int i = 0; i < num_threads; i++)
			{
				threads.emplace_back(atomic_add, num_threads);
			}

			for (auto& t : threads) t.join();

			auto d = high_resolution_clock::now() - t;

			std::cout << num_threads << std::right << std::setw(23) << "Thread(atomic) Sum = " << std::left << std::setw(15) << atomic_sum << "\n";
			std::cout << std::right << std::setw(35) << duration_cast<milliseconds>(d).count() << " msecs\n";
		}
	}

	{ // 멀티 스레드 (optimal)
		for (auto num_threads : { 1,2,4,6,8 }) {
			sum = 0;
			std::vector<std::thread> threads;
			auto t = high_resolution_clock::now();

			for (int i = 0; i < num_threads; i++)
			{
				threads.emplace_back(optimal_add, num_threads);
			}

			for (auto& t : threads) t.join();

			auto d = high_resolution_clock::now() - t;

			std::cout << num_threads << std::right << std::setw(23) << "Thread(optimal) Sum = " << std::left << std::setw(15) << sum << "\n";
			std::cout << std::right << std::setw(35) << duration_cast<milliseconds>(d).count() << " msecs\n";
		}
	}

	{ // 멀티 스레드 (array)
		for (auto num_threads : { 1,2,4,6,8 }) {
			for (auto& s : array_sum) s = 0;

			std::vector<std::thread> threads;
			auto t = high_resolution_clock::now();

			for (int i = 0; i < num_threads; i++)
			{
				threads.emplace_back(array_add, num_threads, i);
			}

			for (auto& t : threads) t.join();

			auto d = high_resolution_clock::now() - t;

			sum = 0;
			for (auto i = 0; i < num_threads; ++i) sum = sum + array_sum[i];

			std::cout << num_threads << std::right << std::setw(23) << "Thread(array) Sum = " << std::left << std::setw(15) << sum << "\n";
			std::cout << std::right << std::setw(35) << duration_cast<milliseconds>(d).count() << " msecs\n";
		}
	}
	
}