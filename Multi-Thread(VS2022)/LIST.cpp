#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>

using namespace std::chrono;

class NODE {
public:
	int key;
	NODE* next;
	std::mutex mtx;

	NODE() { next = nullptr; }
	NODE(int key_value) : key(key_value), next(nullptr) {}
	~NODE() {}
	void lock() {
		mtx.lock();
	}
	void unlock() {
		mtx.unlock();
	}
};

class DUMMY_MUTEX {
public:
	void lock() {};
	void unlock() {};
};

class CLIST {
	NODE* head, * tail;
	std::mutex mtx;
public:
	CLIST() {
		head = new NODE{ std::numeric_limits<int>::min() };
		tail = new NODE{ std::numeric_limits<int>::max() };
		head->next = tail;
	}
	~CLIST() {}

	void clear() {
		while (head->next != tail) {
			NODE* temp = head->next;
			head->next = temp->next;
			delete temp;
		}
	}

	bool Add(int key) {
		mtx.lock();
		NODE* pred = head;
		NODE* curr = pred->next;
		while (curr->key < key) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == key)
		{
			mtx.unlock();
			return false;
		}
		else
		{
			NODE* new_node = new NODE{ key };
			pred->next = new_node;
			new_node->next = curr;
			mtx.unlock();
			return true;
		}
	}

	bool Remove(int key) {
		mtx.lock();
		NODE* pred = head;
		NODE* curr = pred->next;
		while (curr->key != key && curr != tail) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == key) {
			pred->next = curr->next;
			delete curr;
			mtx.unlock();
			return true;
		}
		else {
			mtx.unlock();
			return false;
		}
	}
	bool Contains(int key) {
		mtx.lock();
		NODE* pred = head;
		NODE* curr = pred->next;
		while (curr->key != key && curr != tail) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == key) {
			mtx.unlock();
			return true;
		}
		else {
			mtx.unlock();
			return false;
		}
	}

	void print20() {
		NODE* curr = head->next;
		std::cout << curr->key;
		curr = curr->next;
		int count = 0;
		while (curr != tail && count < 19) {
			std::cout << ", " << curr->key;
			curr = curr->next;
			count++;
		}
		std::cout << std::endl;
	}
};

class FLIST {
	NODE* head, * tail;
public:
	FLIST() {
		head = new NODE{ std::numeric_limits<int>::min() };
		tail = new NODE{ std::numeric_limits<int>::max() };
		head->next = tail;
	}
	~FLIST() {}

	void clear() {
		while (head->next != tail) {
			NODE* temp = head->next;
			head->next = temp->next;
			delete temp;
		}
	}

	bool Add(int key) {
		head->lock();
		NODE* pred = head;
		NODE* curr = pred->next;
		curr->lock();
		while (curr->key < key) {
			pred->unlock();
			pred = curr;
			curr = curr->next;
			curr->lock();
		}

		if (curr->key == key)
		{
			pred->unlock(); curr->unlock();
			return false;
		}
		else
		{
			NODE* new_node = new NODE{ key };
			pred->next = new_node;
			new_node->next = curr;
			pred->unlock(); curr->unlock();
			return true;
		}
	}

	bool Remove(int key) {
		head->lock();
		NODE* pred = head;
		NODE* curr = pred->next;
		curr->lock();
		while (curr->key != key && curr != tail) {
			pred->unlock();
			pred = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->key == key) {
			pred->next = curr->next;
			curr->unlock(); pred->unlock();
			delete curr;
			return true;
		}
		else {
			pred->unlock(); curr->unlock();
			return false;
		}
	}
	bool Contains(int key) {
		head->lock();
		NODE* pred = head;
		NODE* curr = pred->next;
		curr->lock();
		while (curr->key != key && curr != tail) {
			pred->unlock();
			pred = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->key == key) {
			pred->unlock(); curr->unlock();
			return true;
		}
		else {
			pred->unlock(); curr->unlock();
			return false;
		}
	}

	void print20() {
		NODE* curr = head->next;
		std::cout << curr->key;
		curr = curr->next;
		int count = 0;
		while (curr != tail && count < 19) {
			std::cout << ", " << curr->key;
			curr = curr->next;
			count++;
		}
		std::cout << std::endl;
	}
};

class OLIST {
	NODE* head, * tail;
public:
	OLIST() {
		head = new NODE{ std::numeric_limits<int>::min() };
		tail = new NODE{ std::numeric_limits<int>::max() };
		head->next = tail;
	}
	~OLIST() {}

	void clear() {
		while (head->next != tail) {
			NODE* temp = head->next;
			head->next = temp->next;
			delete temp;
		}
	}

	bool Add(int key) {
		NODE* pred = head;
		NODE* curr = pred->next;
		while (curr->key < key) {
			pred = curr;
			curr = curr->next;
		}

		pred->lock(); curr->lock();
		if (curr->key == key)
		{
			pred->unlock(); curr->unlock();
			return false;
		}
		else
		{
			NODE* new_node = new NODE{ key };
			pred->next = new_node;
			new_node->next = curr;
			pred->unlock(); curr->unlock();
			return true;
		}
	}

	bool Remove(int key) {
		NODE* pred = head;
		NODE* curr = pred->next;
		while (curr->key != key && curr != tail) {
			pred = curr;
			curr = curr->next;
		}

		pred->lock(); curr->lock();
		if (curr->key == key) {
			pred->next = curr->next;
			curr->unlock(); pred->unlock();
			delete curr;
			return true;
		}
		else {
			pred->unlock(); curr->unlock();
			return false;
		}
	}
	bool Contains(int key) {
		NODE* pred = head;
		NODE* curr = pred->next;
		while (curr->key != key && curr != tail) {
			pred = curr;
			curr = curr->next;
		}

		pred->lock(); curr->lock();
		if (curr->key == key) {
			pred->unlock(); curr->unlock();
			return true;
		}
		else {
			pred->unlock(); curr->unlock();
			return false;
		}
	}

	void print20() {
		NODE* curr = head->next;
		std::cout << curr->key;
		curr = curr->next;
		int count = 0;
		while (curr != tail && count < 19) {
			std::cout << ", " << curr->key;
			curr = curr->next;
			count++;
		}
		std::cout << std::endl;
	}
};

constexpr int MAX_THREADS = 16;
constexpr int NUM_TEST = 400'0000;

OLIST my_set;

void benchmark(int num_thread)
{
	const int LOOP = NUM_TEST / num_thread;
	for (int i = 0;i < LOOP;i++) {
		int value = rand() % 1000;
		int op = rand() % 3;
		switch (op) {
		case 0:
			my_set.Add(value);
			break;
		case 1:
			my_set.Remove(value);
			break;
		case 2:
			my_set.Contains(value);
			/*if (my_set.Contains(value)) {
				std::cout << "O, ";
			}
			else
			{
				std::cout << "X, ";
			}*/
			break;
		default: std::cout << "Error\n";
			exit(-1);
		}
	}
}

int main()
{
	for (int num_thread = 1; num_thread <= MAX_THREADS; num_thread *= 2) {
		std::vector<std::thread> threads;
		auto t = high_resolution_clock::now();
		for (int j = 0; j < num_thread; j++)
		{
			threads.emplace_back(benchmark, num_thread);
		}
		for (auto& t : threads) t.join();
		auto d = high_resolution_clock::now() - t;
		auto exec_ms = duration_cast<milliseconds>(d).count();
		my_set.print20();
		std::cout << "Thread: " << num_thread <<", Time: " << exec_ms << " msecs\n";
		my_set.clear();
	}
}