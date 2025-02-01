#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <vector>
#include <stdexcept>
#include <queue>

struct ElevatorEvent {
    struct tm timestamp;            // timestamp of the request in tm format
    int floor;                      // Floor number
    std::string floorButton;        // "up", "down", or empty string ""
    int carButton;                  // Holds an integer value for the car button

    // Expected file structure:
    // current time in secs current time in ms
    // 1617751896 250 
    // 1617751900 500
    // 1617751920 750

    // Constructor with validation
    ElevatorEvent(struct tm t, int f, std::string fb, int cb) 
        : timestamp(t), floor(f), carButton(cb) {
        
        // Validate floorButton input
        if (fb != "up" && fb != "down" && !fb.empty()) {
            throw std::invalid_argument("Invalid floor button value. Must be 'up', 'down', or empty.");
        }
        floorButton = std::move(fb);
    }

    // Display function for debugging
    std::string display() const {
        std::ostringstream oss;
        oss << "Time: " << std::put_time(&timestamp, "%Y-%m-%d %H:%M:%S") 
            << ", Floor: " << floor 
            << ", Floor Button: " << (floorButton.empty() ? "None" : floorButton) 
            << ", Car Button: " << carButton;
        return oss.str();
    }
};

// Thread-safe Scheduler for ElevatorEvent
template <typename Type> class Scheduler {
private:
    std::queue<Type> queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    Scheduler() = default;

    void put(Type item) {
        std::unique_lock<std::mutex> lock(mtx);
        queue.push(item);
        cv.notify_all();
    }

    Type get() {
        std::unique_lock<std::mutex> lock(mtx);
        while (queue.empty()) cv.wait(lock);
        Type item = queue.front();
        queue.pop();
        cv.notify_all();
        return item;
    }
};

// Floor class to generate ElevatorEvents
template <typename Type> class Floor
{
private:
    std::string name;
    Scheduler<Type>& scheduler;

    struct tm generateRandomTM() {
        struct tm randomTime;

        // Randomize year (1900 to 2023)
        randomTime.tm_year = rand() % 124 + 100;  // tm_year is years since 1900

        // Randomize month (0 to 11)
        randomTime.tm_mon = rand() % 12;

        // Randomize day (1 to 31, depending on month)
        randomTime.tm_mday = rand() % 31 + 1;

        // Randomize hour (0 to 23)
        randomTime.tm_hour = rand() % 24;

        // Randomize minute (0 to 59)
        randomTime.tm_min = rand() % 60;

        // Randomize second (0 to 59)
        randomTime.tm_sec = rand() % 60;

        // Normalize the tm structure (adjusts tm_wday, tm_yday, etc.)
        mktime(&randomTime);

        return randomTime;
    }

    // Function to generate random ElevatorEvent (for debugging purposes...)
    ElevatorEvent generateEvent(int floorNumber) {
        // Get random timestamp
        struct tm timestamp;
        timestamp = generateRandomTM();

        // Generate random floor button
        std::string floorButton;
        switch (rand() % 2) {
            case 0: floorButton = "up"; break;
            case 1: floorButton = "down"; break;
            default: floorButton = "";
        }

        // Generate random car button between 1 and 10
        int carButton = rand() % 10 + 1;

        return ElevatorEvent(timestamp, floorNumber, floorButton, carButton);
    }

public:
    Floor( Scheduler<Type>& a_scheduler ) : name(), scheduler(a_scheduler) {}
    
    void operator()( const std::string& name, int floorNumber) {
	    std::cout << name << "(" << std::this_thread::get_id() << ") generated task " << std::endl;
        ElevatorEvent item = generateEvent(floorNumber);
	    scheduler.put(item);
	    std::cout << name << "(" << std::this_thread::get_id() << ") put in scheduler " << item.display() << std::endl;
	    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
};

// Elevator class to process Scheduler Requests
template <typename Type> class Elevator
{
private:
    std::string name;
    Scheduler<Type>& scheduler;

public:
    Elevator( Scheduler<Type>& a_scheduler ) : name(), scheduler(a_scheduler) {}
    
    void operator()( const std::string& name ) {
        while( true ) {
            std::cout << name << "(" << std::this_thread::get_id() << ") ready to process task " << std::endl;
            ElevatorEvent item = scheduler.get();
            std::cout << name << "(" << std::this_thread::get_id() << ") processing task. " << item.display() << std::endl;
            std::this_thread::sleep_for( std::chrono::seconds( 1) );
            std::cout << name << "(" << std::this_thread::get_id() << ") elevator processed task. " << item.display() << std::endl;
        }
    }
};

int main() {

    // Create a Scheduler object
    Scheduler<ElevatorEvent> scheduler;

    // Create Floor objects
    Floor<ElevatorEvent> floor(scheduler);
    Floor<ElevatorEvent> floor2(scheduler);
    Floor<ElevatorEvent> floor3(scheduler);
    Floor<ElevatorEvent> floor4(scheduler);
    Floor<ElevatorEvent> floor5(scheduler);

    // Create a Consumer object
    Elevator<ElevatorEvent> elevator(scheduler);

    // Create a thread for the Floor object
    std::thread floorThread(floor, "test", 1);
    std::thread floorThread2(floor2, "Floor2", 2);
    std::thread floorThread3(floor3, "Floor3", 3);
    std::thread floorThread4(floor4, "Floor4", 4);
    std::thread floorThread5(floor5, "Floor5", 5);

    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

    // Create a thread for the Consumer object
    std::thread elevatorThread(elevator, "Elevator");

    // Join the threads
    floorThread.join();
    floorThread2.join();
    floorThread3.join();
    floorThread4.join();
    floorThread5.join();
    elevatorThread.join();

    return 0;
}