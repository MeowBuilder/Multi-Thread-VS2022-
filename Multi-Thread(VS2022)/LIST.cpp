#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <queue>
#include <array>

using namespace std::chrono;

constexpr int MAX_THREADS = 16;
constexpr int NUM_TEST = 400'0000;
constexpr int RANGE = 1000;

class DUMMY_MUTEX {
public:
	void lock() {};
	void unlock() {};
};

class NODE {
public:
	int key;
	NODE* next;
	bool removed = false;
	std::mutex mtx;

	NODE(int key_value) : key(key_value), next(nullptr) {}
	void lock() {
		mtx.lock();
	}
	void unlock() {
		mtx.unlock();
	}
};

class CLIST {
	NODE* head, * tail;
	std::mutex mtx;
public:
	CLIST() {
		std::cout << "Testing Course Grain Synchronization List\n";
		head = new NODE{ std::numeric_limits<int>::min() };
		tail = new NODE{ std::numeric_limits<int>::max() };
		head->next = tail;
	}
	~CLIST() {}

	void clear() {
		NODE* current = head->next;
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
		std::cout << "Testing Fine Grain Synchronization List\n";
		head = new NODE{ std::numeric_limits<int>::min() };
		tail = new NODE{ std::numeric_limits<int>::max() };
		head->next = tail;
	}
	~FLIST() {}

	void clear() {
		NODE* current = head->next;
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

class MEMORY_POOL {
private:
	std::queue<NODE*> get_pool;	
	std::queue<NODE*> free_pool;
public:
	MEMORY_POOL() {
		//for (int i = 0;i < NUM_TEST / 3; ++i) {
		//	get_pool.push(new NODE(0));
		//}
	}
	~MEMORY_POOL() {
		while (!get_pool.empty()) {
			delete get_pool.front();
			get_pool.pop();
		}
		while (!free_pool.empty()) {
			delete free_pool.front();
			free_pool.pop();
		}
	}

	NODE* get_node(int value) {
		if (get_pool.empty()) {
			return new NODE(value);
		}
		else {
			NODE* node = get_pool.front();
			get_pool.pop();
			node->key = value;
			node->next = nullptr;
			node->removed = false;
			return node;
		}
	}
	void free_node(NODE* node) {
		free_pool.push(node);
	}
	void recycle_nodes() {
		get_pool = std::move(free_pool);
	}
};

MEMORY_POOL memory_pool[MAX_THREADS];
thread_local int thread_id = 999;

class OLIST {
	NODE* head, * tail;
public:
	OLIST() {
		std::cout << "Testing Optimistic Synchronization List\n";
		head = new NODE{ std::numeric_limits<int>::min() };
		tail = new NODE{ std::numeric_limits<int>::max() };
		head->next = tail;
	}
	~OLIST() {}

	void clear() {
		NODE* current = head->next;
		while (head->next != tail) {
			NODE* temp = head->next;
			head->next = temp->next;
			delete temp;
		}
	}

	bool validate(NODE* pred, NODE* curr) {
		NODE* node = head;
		while (node->key <= pred->key) {
			if (node == pred) return (pred->next == curr);
			node = node->next;
		}
		return false;
	}

	bool Add(int key) {
		while (true)
		{
			NODE* pred = head;
			NODE* curr = pred->next;
			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();
			if (false == validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key)
			{
				pred->unlock(); curr->unlock();
				return false;
			}
			else
			{
				NODE* new_node = memory_pool[thread_id].get_node(key);
				new_node->next = curr;
				pred->next = new_node;
				pred->unlock(); curr->unlock();
				return true;
			}
		}
	}

	bool Remove(int key) {
		while (true) {
			NODE* pred = head;
			NODE* curr = pred->next;
			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}
			//while (curr->key != key && curr != tail) { // 왜인지 멀티스레드에서 코어 늘어날수록 느려짐 -> 탐색이 오래 걸림(Tail까지 탐색) -> 탐색이 오래 걸리면 validate 실패확률 증가
			//	pred = curr;
			//	curr = curr->next;
			//}

			pred->lock(); curr->lock();
			if (!validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key) {
				pred->next = curr->next;
				curr->unlock(); pred->unlock();
				memory_pool[thread_id].free_node(curr);
				return true;
			}
			else {
				pred->unlock(); curr->unlock();
				return false;
			}
			//if (curr->key != key) {
			//	pred->unlock(); curr->unlock();
			//	return false;
			//}
			//else {
			//	pred->next = curr->next;
			//	curr->unlock(); pred->unlock();
			//	memory_pool[thread_id].free_node(curr);
			//	return true;
			//}
		}
	}
	bool Contains(int key) {
		while (true) {
			NODE* pred = head;
			NODE* curr = pred->next;
			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}
			//while (curr->key != key && curr != tail) {
			//	pred = curr;
			//	curr = curr->next;
			//}

			pred->lock(); curr->lock();
			if (!validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
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

class ZLIST {
	NODE* head, * tail;
public:
	ZLIST() {
		std::cout << "Testing lazy synchronization List\n";
		head = new NODE{ std::numeric_limits<int>::min() };
		tail = new NODE{ std::numeric_limits<int>::max() };
		head->next = tail;
	}
	~ZLIST() {}

	void clear() {
		NODE* current = head->next;
		while (head->next != tail) {
			NODE* temp = head->next;
			head->next = temp->next;
			delete temp;
		}
	}

	bool validate(NODE* pred, NODE* curr) {
		return ((!pred->removed) && (!curr->removed) && (pred->next == curr));
	}

	bool Add(int key) {
		while (true)
		{
			NODE* pred = head;
			NODE* curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();
			if (false == validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key)
			{
				pred->unlock(); curr->unlock();
				return false;
			}
			else
			{
				NODE* new_node = memory_pool[thread_id].get_node(key);
				new_node->next = curr;
				pred->next = new_node;
				pred->unlock(); curr->unlock();
				return true;
			}
		}
	}

	bool Remove(int key) {
		while (true) {
			NODE* pred = head;
			NODE* curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();
			if (!validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key) {
				curr->removed = true;
				pred->next = curr->next;
				curr->unlock(); pred->unlock();
				memory_pool[thread_id].free_node(curr);
				return true;
			}
			else {
				pred->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool Contains(int key) {
		while (true) {
			NODE* pred = head;
			NODE* curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			return ((curr->key == key) && (!curr->removed));
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

class NODE_SP {
public:
	int key;
	std::shared_ptr<NODE_SP> next;
	bool removed = false;
	std::mutex mtx;

	NODE_SP(int key_value) : key(key_value), next(nullptr) {}
	void lock() { mtx.lock(); }
	void unlock() { mtx.unlock(); }
};

class ZLISTSP {
	std::shared_ptr<NODE_SP> head, tail;
public:
	ZLISTSP() {
		std::cout << "Testing lazy synchronization List\n";
		head = std::make_shared<NODE_SP>(std::numeric_limits<int>::min());
		tail = std::make_shared<NODE_SP>(std::numeric_limits<int>::max());
		head->next = tail;
	}
	~ZLISTSP() {}

	void clear() {
		head->next = tail;
	}

	bool validate(const std::shared_ptr<NODE_SP> &pred, const std::shared_ptr<NODE_SP> &curr) {
		return ((!pred->removed) && (!curr->removed) && (pred->next == curr));
	}

	bool Add(int key) {
		while (true)
		{
			std::shared_ptr<NODE_SP> pred = head;
			std::shared_ptr<NODE_SP> curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();
			if (false == validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key)
			{
				pred->unlock(); curr->unlock();
				return false;
			}
			else
			{
				std::shared_ptr<NODE_SP> new_node = std::make_shared<NODE_SP>(key);
				new_node->next = curr;
				pred->next = new_node;
				pred->unlock(); curr->unlock();
				return true;
			}
		}
	}

	bool Remove(int key) {
		while (true) {
			std::shared_ptr<NODE_SP> pred = head;
			std::shared_ptr<NODE_SP> curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();
			if (!validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key) {
				curr->removed = true;
				pred->next = curr->next;
				curr->unlock(); pred->unlock();
				return true;
			}
			else {
				pred->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool Contains(int key) {
		while (true) {
			std::shared_ptr<NODE_SP> pred = head;
			std::shared_ptr<NODE_SP> curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			return ((curr->key == key) && (!curr->removed));
		}
	}


	void print20() {
		std::shared_ptr<NODE_SP> curr = head->next;
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

class NODE_ASP {
public:
	int key;
	std::atomic<std::shared_ptr<NODE_ASP>> next;
	bool removed = false;
	std::mutex mtx;

	NODE_ASP(int key_value) : key(key_value), next(nullptr) {}
	void lock() { mtx.lock(); }
	void unlock() { mtx.unlock(); }
};

class ZLISTASP {
	std::atomic<std::shared_ptr<NODE_ASP>> head, tail;
public:
	ZLISTASP() {
		std::cout << "Testing lazy synchronization List\n";
		head = std::make_shared<NODE_ASP>(std::numeric_limits<int>::min());
		tail = std::make_shared<NODE_ASP>(std::numeric_limits<int>::max());
		head.load()->next = tail.load();
	}
	~ZLISTASP() {}

	void clear() {
		head.load()->next = tail.load();
	}

	bool validate(const std::shared_ptr<NODE_ASP>& pred, const std::shared_ptr<NODE_ASP>& curr) {
		return ((!pred->removed) && (!curr->removed) && (pred->next.load() == curr));
	}

	bool Add(int key) {
		while (true)
		{
			std::shared_ptr<NODE_ASP> pred = head;
			std::shared_ptr<NODE_ASP> curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();
			if (false == validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key)
			{
				pred->unlock(); curr->unlock();
				return false;
			}
			else
			{
				std::shared_ptr<NODE_ASP> new_node = std::make_shared<NODE_ASP>(key);
				new_node->next = curr;
				pred->next.load() = new_node;
				pred->unlock(); curr->unlock();
				return true;
			}
		}
	}

	bool Remove(int key) {
		while (true) {
			std::shared_ptr<NODE_ASP> pred = head.load();
			std::shared_ptr<NODE_ASP> curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();
			if (!validate(pred, curr)) {
				pred->unlock(); curr->unlock();
				continue;
			}

			if (curr->key == key) {
				curr->removed = true;
				pred->next.load() = curr->next;
				curr->unlock(); pred->unlock();
				return true;
			}
			else {
				pred->unlock(); curr->unlock();
				return false;
			}
		}
	}
	bool Contains(int key) {
		while (true) {
			std::shared_ptr<NODE_ASP> pred = head;
			std::shared_ptr<NODE_ASP> curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			return ((curr->key == key) && (!curr->removed));
		}
	}


	void print20() {
		std::shared_ptr<NODE_ASP> curr = head.load()->next;
		std::cout << curr->key;
		curr = curr->next;
		int count = 0;
		while (curr != tail.load() && count < 19) {
			std::cout << ", " << curr->key;
			curr = curr->next;
			count++;
		}
		std::cout << std::endl;
	}
};

ZLISTSP my_set;

class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

std::array<std::vector<HISTORY>, MAX_THREADS> history;

void check_history(int num_threads)
{
	std::array <int, RANGE> survive = {};
	std::cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		std::cout << "No history.\n";
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
	for (int i = 0; i < RANGE; ++i) {
		int val = survive[i];
		if (val < 0) {
			std::cout << "ERROR. The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			std::cout << "ERROR. The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (my_set.Contains(i)) {
				std::cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == my_set.Contains(i)) {
				std::cout << "ERROR. The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	std::cout << " OK\n";
}

void benchmark_check(int num_threads, int th_id)
{
	thread_id = th_id;
	for (int i = 0; i < NUM_TEST / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % RANGE;
			history[th_id].emplace_back(0, v, my_set.Add(v));
			break;
		}
		case 1: {
			int v = rand() % RANGE;
			history[th_id].emplace_back(1, v, my_set.Remove(v));
			break;
		}
		case 2: {
			int v = rand() % RANGE;
			history[th_id].emplace_back(2, v, my_set.Contains(v));
			break;
		}
		}
	}
	memory_pool[thread_id].recycle_nodes();
}

void benchmark(int num_thread, int tid)
{
	thread_id = tid;
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
			break;
		default: std::cout << "Error\n";
			exit(-1);
		}
	}
	memory_pool[thread_id].recycle_nodes();
}

int main()
{
	std::cout << "Strting Error Check.\n";
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		std::vector<std::thread> threads;
		for (auto& h : history) h.clear();
		auto start_time = high_resolution_clock::now();
		for (int j = 0; j < num_threads; ++j) {
			threads.emplace_back(benchmark_check, num_threads, j);
		}
		for (auto& thread : threads) {
			thread.join();
		}
		auto end_time = high_resolution_clock::now();
		auto elapsed = end_time - start_time;
		auto exec_ms = duration_cast<milliseconds>(elapsed).count();
		my_set.print20();
		std::cout << "Threads: " << num_threads << ", Time: " << exec_ms << " ms\n";
		check_history(num_threads);
		my_set.clear();
	}

	std::cout << "Strting Performance Check.\n";
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		std::vector<std::thread> threads;
		auto start_time = high_resolution_clock::now();
		for (int j = 0; j < num_threads; ++j) {
			threads.emplace_back(benchmark, num_threads, j);
		}
		for (auto& thread : threads) {
			thread.join();
		}
		auto end_time = high_resolution_clock::now();
		auto elapsed = end_time - start_time;
		auto exec_ms = duration_cast<milliseconds>(elapsed).count();
		my_set.print20();
		std::cout << "Threads: " << num_threads << ", Time: " << exec_ms << " ms\n";
		my_set.clear();
	}
}