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
#include <string>
#include <map>

enum class ElevatorState { Idle, MovingUp, MovingDown, DoorOpening, DoorOpen, DoorClosing };
enum class Direction { Up, Down, Idle };

struct ElevatorEvent {
    struct tm timestamp;
    int floor;
    std::string floorButton;
    int floorsToMove; // Adjusted to represent how many floors to move instead of target floor

    ElevatorEvent(struct tm t, int f, std::string fb, int cb)
        : timestamp(t), floor(f), floorButton(std::move(fb)), floorsToMove(cb) {}

    std::string display() const {
        std::ostringstream oss;
        oss << "[Floor Subsystem] Request sent: Floor " << floor
            << " going " << (floorButton.empty() ? "None" : floorButton)
            << " by " << floorsToMove << " floors";
        return oss.str();
    }
};

template <typename Type>
class Scheduler {
private:
    std::queue<Type> queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    void put(Type item) {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "[Scheduler] Assign " << item.display() << " to Elevator" << std::endl;
        queue.push(item);
        cv.notify_all();
    }

    Type get() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !queue.empty(); });
        Type item = queue.front();
        queue.pop();
        return item;
    }
};

template <typename Type>
class Floor {
private:
    std::string filename;
    Scheduler<Type>& scheduler;
    Scheduler<Type>& elevatorNotifier;

public:
    Floor(const std::string& file, Scheduler<Type>& sched, Scheduler<Type>& notifier)
        : filename(file), scheduler(sched), elevatorNotifier(notifier) {}

    void operator()() {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        std::cout << "[Scheduler] Request input from the Floor Subsystem" << std::endl;
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string timeStr, floorButton;
            int floor, floorsToMove;

            if (!(iss >> timeStr >> floor >> floorButton >> floorsToMove)) {
                std::cerr << "Incorrect line format: " << line << std::endl;
                continue;
            }

            struct tm timestamp = {};
            std::istringstream timeStream(timeStr);
            timeStream >> std::get_time(&timestamp, "%H:%M:%S");

            scheduler.put(Type(timestamp, floor, floorButton, floorsToMove));
        }
    }
};

template <typename Type>
class Elevator {
private:
    Scheduler<Type>& scheduler;
    Scheduler<Type>& floorNotifier;
    ElevatorState state;
    Direction direction;
    int currentFloor;

    void moveByFloors(int floorsToMove, std::string directionStr) {
        int targetFloor = (directionStr == "Up") ? currentFloor + floorsToMove : currentFloor - floorsToMove;
        std::cout << "[Elevator] Starting at Floor " << currentFloor << std::endl;
        while (currentFloor != targetFloor) {
            if (currentFloor < targetFloor) {
                currentFloor++;
                state = ElevatorState::MovingUp;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "[Elevator] Moving up: " << currentFloor << std::endl;
            } else {
                currentFloor--;
                state = ElevatorState::MovingDown;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "[Elevator] Moving down: " << currentFloor << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        doorOperations();
        std::cout << "[Elevator] Notify Scheduler" << std::endl;
    }

    void doorOperations() {
        state = ElevatorState::DoorOpening;
        std::cout << "[Elevator] Doors opening at floor: " << currentFloor << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        state = ElevatorState::DoorOpen;
        std::cout << "[Elevator] Boarding at floor: " << currentFloor << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        state = ElevatorState::DoorClosing;
        std::cout << "[Elevator] Doors closing at floor: " << currentFloor << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        state = ElevatorState::Idle;
    }

public:
    ElevatorState getState() const { return state; }
    Elevator(Scheduler<Type>& sched, Scheduler<Type>& notifier)
        : scheduler(sched), floorNotifier(notifier), state(ElevatorState::Idle), direction(Direction::Idle), currentFloor(1) {}

    void operator()() {
        while (true) {
            std::cout << "[Elevator] Waiting for next task..." << std::endl;
            ElevatorEvent item = scheduler.get();
            std::cout << "[Elevator] Received: " << item.display() << std::endl;
            moveToFloor(item.floor); // Move to pickup floor
            floorNotifier.put(item);
            moveByFloors(item.floorsToMove, item.floorButton); // Adjust movement by floors, not direct floor
            floorNotifier.put(item);
            std::cout << "[Scheduler] Request completed" << std::endl;
        }
    }

    void moveToFloor(int targetFloor) {
        std::cout << "[Elevator] Moving to pickup floor " << targetFloor << std::endl;
        
        // Calculate the number of floors to move based on the direction
        int floorsToMove = std::abs(currentFloor - targetFloor); // Absolute difference in floors
        std::string directionStr;
    
        if (currentFloor < targetFloor) {
            directionStr = "Up";  // If the current floor is lower, moving up
        } else {
            directionStr = "Down"; // If the current floor is higher, moving down
        }
    
        // Call moveByFloors to move the elevator
        moveByFloors(floorsToMove, directionStr);
    
        doorOperations();
    }

    int getCurrentFloor() const {
        return currentFloor;
    }
};
