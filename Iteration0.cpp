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
    std::chrono::milliseconds time; // Time stored in milliseconds
    int floor;                      // Floor number
    std::string floorButton;        // "up", "down", or empty string ""
    int carButton;                  // Holds an integer value for the car button

    // Constructor with validation
    ElevatorEvent(std::chrono::milliseconds t, int f, std::string fb, int cb) 
        : time(t), floor(f), carButton(cb) {
        
        // Validate floorButton input
        if (fb != "up" && fb != "down" && !fb.empty()) {
            throw std::invalid_argument("Invalid floor button value. Must be 'up', 'down', or empty.");
        }
        floorButton = std::move(fb);
    }

    // Function to format time as hh:mm:ss.mmm
    std::string getFormattedTime() const {
        using namespace std::chrono;

        // Extract total milliseconds as a raw value
        auto total_ms = time.count();

        // Compute hours, minutes, seconds, and milliseconds
        int hours = total_ms / (1000 * 60 * 60);
        total_ms %= (1000 * 60 * 60);
        int minutes = total_ms / (1000 * 60);
        total_ms %= (1000 * 60);
        int seconds = total_ms / 1000;
        int milliseconds = total_ms % 1000;

        // Format output
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << seconds << "."
            << std::setw(3) << std::setfill('0') << milliseconds;

        return oss.str();
    }

    // Display function for debugging
    void display() const {
        std::cout << "Time: " << getFormattedTime() 
                  << ", Floor: " << floor 
                  << ", Floor Button: " << (floorButton.empty() ? "None" : floorButton) 
                  << ", Car Button: " << carButton 
                  << std::endl;
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

    Type generateEvent() { // Should not be of Type Type.... 
        // Generate random time between 0 and 1000 milliseconds
        std::chrono::milliseconds time(rand() % 1000);

        // Generate random floor between 1 and 10
        int floor = rand() % 10 + 1;

        // Generate random floor button
        std::string floorButton;
        switch (rand() % 2) {
            case 0: floorButton = "up"; break;
            case 1: floorButton = "down"; break;
            default: floorButton = "";
        }

        // Generate random car button between 1 and 10
        int carButton = rand() % 10 + 1;

        return Type(time, floor, floorButton, carButton);
    }

public:
    Floor( Scheduler<Type>& a_scheduler ) : name(), scheduler(a_scheduler) {}
    
    void operator()( const std::string& name ) {
	    std::cout << name << "(" << std::this_thread::get_id() << ") produced " << std::endl;
	    scheduler.put(generateEvent());
	    std::cout << name << "(" << std::this_thread::get_id() << ") put in scheduler " << std::endl;
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
	    std::cout << name << "(" << std::this_thread::get_id() << ") ready to consume " << std::endl;
	    Type item = scheduler.get();
	    std::cout << name << "(" << std::this_thread::get_id() << ") consumed " << std::endl;
	    std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    }
};

int main() {

    // Create a Scheduler object
    Scheduler<ElevatorEvent> scheduler;

    // Create a Floor object
    Floor<ElevatorEvent> floor(scheduler);

    // Create a Consumer object
    Elevator<ElevatorEvent> elevator(scheduler);

    // Create a thread for the Floor object
    std::thread floorThread(floor, "Floor");

    // Create a thread for the Consumer object
    std::thread elevatorThread(elevator, "Elevator");

    // Join the threads
    floorThread.join();
    elevatorThread.join();

    return 0;
}