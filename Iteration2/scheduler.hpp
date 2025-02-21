/*
 * SYSC 3303 B1-Group 1 
 * Created by Hasan Suriya
 * Reviewed by Abdullah Arid
 * 
 * This header file defines a class for the Scheduler subsystem. The 
 * Scheduler class schedules items, it includes methods to add an 
 * item to the queue and retrieve an item from the queue.
 * 
*/

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

template <typename Type>
class Scheduler {
private:
    std::queue<Type> queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    // Adds an item to the queue and notifies waiting threads.
    void put(Type item) {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "[Scheduler] Assign " << item.display() << " to Elevator" << std::endl;
        queue.push(item);
        cv.notify_all();
    }

    // Retrieves an item from the queue.
    Type get() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !queue.empty(); });
        Type item = queue.front();
        queue.pop();
        return item;
    }
};

#endif // SCHEDULER_H
