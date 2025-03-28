#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <random>
#include <mutex>
#include <chrono>
#include <algorithm>

using namespace std;

const int DATA_SIZE = 1000000000;
const int NUM_THREADS = 1;
const int DIVISOR = 19;

// Data generation
vector<int> generateData(int size) {
    vector<int> data(size);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution dist(0, 99999);

    for (int i = 0; i < size; ++i) {
        data[i] = dist(gen);
    }

    return data;
}

// Without parallelization
void findDivisibleWithoutParallel(const vector<int>& data, int& count, int& minElement) {
    count = 0;
    minElement = INT_MAX;
    for (const auto value : data) {
        if (value % DIVISOR == 0) {
            ++count;
            minElement = min(minElement, value);
        }
    }
}

// With blocking primitives
void findDivisibleWithMutex(const vector<int>& data, int& count, int& minElement) {
    mutex mtx;
    count = 0;
    minElement = INT_MAX;
    vector<thread> threads;

    auto task = [&](int start, int end) {
        int localCount = 0;
        int localMin = INT_MAX;
        for (int i = start; i < end; ++i) {
            if (data[i] % DIVISOR == 0) {
                ++localCount;
                localMin = min(localMin, data[i]);
            }
        }
        lock_guard lock(mtx);
        count += localCount;
        minElement = min(minElement, localMin);
    };

    const int chunkSize = DATA_SIZE / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; ++i) {
        int start = i * chunkSize;
        int end = (i == NUM_THREADS - 1) ? DATA_SIZE : start + chunkSize;
        threads.emplace_back(task, start, end);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

// Optimized With atomic variables and CAS
void findDivisibleWithAtomic(const vector<int>& data, atomic<int>& count, atomic<int>& minElement) {
    vector<thread> threads;

    auto task = [&](const int start, const int end) {
        int localCount = 0;
        int localMin = INT_MAX;

        for (int i = start; i < end; ++i) {
            if (data[i] % DIVISOR == 0) {
                ++localCount;
                localMin = min(localMin, data[i]);
            }
        }
        count.fetch_add(localCount);

        int currentMin = minElement.load();
        while (localMin < currentMin &&
               !minElement.compare_exchange_weak(currentMin, localMin)) {
               }
    };

    int chunkSize = DATA_SIZE / NUM_THREADS;
    for (int i = 0; i < NUM_THREADS; ++i) {
        int start = i * chunkSize;
        int end = (i == NUM_THREADS - 1) ? DATA_SIZE : start + chunkSize;
        threads.emplace_back(task, start, end);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

int main() {
    vector<int> data = generateData(DATA_SIZE);

    int count = 0, minElement = 0;
    atomic atomicCount(0);
    atomic atomicMinElement(INT_MAX);

    // Without parallelization
    auto start = chrono::high_resolution_clock::now();
    findDivisibleWithoutParallel(data, count, minElement);
    auto end = chrono::high_resolution_clock::now();
    double elapsed = chrono::duration<double>(end - start).count();
    cout << "[*] Without parallelization\n";
    cout << "Found: " << count << " elements, minimum: " << minElement
         << ", time: " << elapsed << " s" << endl;

    // With mutex
    start = chrono::high_resolution_clock::now();
    findDivisibleWithMutex(data, count, minElement);
    end = chrono::high_resolution_clock::now();
    elapsed = chrono::duration<double>(end - start).count();
    cout << "[*] With mutex\n";
    cout << "Found: " << count << " elements, minimum: " << minElement
         << ", time: " << elapsed << " s" << endl;


    // With atomic variables
    start = chrono::high_resolution_clock::now();
    findDivisibleWithAtomic(data, atomicCount, atomicMinElement);
    end = chrono::high_resolution_clock::now();
    elapsed = chrono::duration<double>(end - start).count();
    cout << "[*] With atomic variables\n";
    cout << "Found: " << atomicCount.load() << " elements, minimum: "
         << atomicMinElement.load() << ", time: "
         << elapsed << " s" << endl;

    return 0;
}
