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
#include <fstream>


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
        : timestamp(t), floor(f), carButton(cb),floorButton(std::move(fb)){
        
        // Validate floorButton input
        //if (fb != "up" && fb != "down" && !fb.empty()) {
            //throw std::invalid_argument("Invalid floor button value. Must be 'up', 'down', or empty.");
        //}
    }

    // Display function for debugging
    std::string display() const {
        std::ostringstream oss;
        oss << "Time: " << std::put_time(&timestamp, "%H:%M:%S") 
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

// Function to read events from a text file
template <typename Type> class Floor {
private:
    std::string filename;
    Scheduler<Type>& scheduler;
    Scheduler<Type>& elevatorNotifier;

public:
    Floor(const std::string& file, Scheduler<Type>& sched, Scheduler<Type>& notifier) : filename(file), scheduler(sched), elevatorNotifier(notifier) {}

    void operator()() {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string timeStr, floorButton;
            int floor, carButton;
            
            if (!(iss >> timeStr >> floor >> floorButton >> carButton)) {
                std::cerr << "Incorrect line format: " << line << std::endl;
                continue;
            }
            
            struct tm timestamp = {};
            std::istringstream timeStream(timeStr);
            timeStream >> std::get_time(&timestamp, "%H:%M:%S");
            
            scheduler.put(Type(timestamp, floor, floorButton, carButton));
        }
        while (true) {
            Type arrivedEvent = elevatorNotifier.get();
            int adjustedFloor = arrivedEvent.carButton;
            std::cout << "Floor thread notified: Elevator arrived at floor " << adjustedFloor << std::endl;
        }
    }
};

// Elevator class to process Scheduler Requests
template <typename Type> class Elevator
{
private:
    //std::string name;
    Scheduler<Type>& scheduler;
    Scheduler<Type>& floorNotifier;

public:
    Elevator( Scheduler<Type>& a_scheduler, Scheduler<Type>& notifier ) : scheduler(a_scheduler), floorNotifier(notifier) {}
    
    void operator()( ) {
        while( true ) {
            std::cout <<"Ready to process task " << std::endl;
            ElevatorEvent item = scheduler.get();
            std::cout <<"Processing task:  " <<item.display() << std::endl;
            std::this_thread::sleep_for( std::chrono::seconds( 1) );
            std::cout <<"Elevator processed task: "  << item.display() << std::endl;

            // Notify floor thread that the elevator has arrived at the floor
            floorNotifier.put(item);
        }
    }
};

int main() {

    // Create a Scheduler object
    Scheduler<ElevatorEvent> scheduler;
    Scheduler<ElevatorEvent> floorNotifier;

    // Create a Floor object
    Floor<ElevatorEvent> floorReader("elevator.txt", scheduler, floorNotifier);
    std::thread floorThread(std::ref(floorReader));

    // Create a Consumer object
    Elevator<ElevatorEvent> elevator(scheduler, floorNotifier);
    std::thread elevatorThread(std::ref(elevator));

    elevatorThread.join();
    floorThread.join();

    return 0;
}