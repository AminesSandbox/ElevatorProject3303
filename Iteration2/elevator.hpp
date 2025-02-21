/*
 * SYSC 3303 B1-Group 1 
 * Created by Hasan Suriya
 * Reviewed by Abdullah Arid
 * 
 * This header file defines a class for the Elevator subsystem. The Elevator
 * class represents an elevator that moves between floors, opens and closes
 * its doors, and notifies the scheduler and floor subsystems of its actions.
 * It includes methods to move the elevator, open and close its doors, and
 * process elevator events.
 * 
*/

#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <iostream>
#include <thread>
#include <chrono>
#include "scheduler.hpp"
#include "elevator_event.hpp"

enum class ElevatorState { Idle, MovingUp, MovingDown, DoorOpening, DoorOpen, DoorClosing };
enum class Direction { Up, Down, Idle };

template <typename Type>
class Elevator {
private:
    Scheduler<Type>& scheduler;
    Scheduler<Type>& floorNotifier;
    ElevatorState state;
    Direction direction;
    int currentFloor;

    /*
    * Moves the elevator to the target floor by incrementing or decrementing
    * the current floor until it reaches the target floor. The elevator state
    * is updated to MovingUp or MovingDown as it moves between floors.
    */
    void moveByFloors(int floorsToMove, const std::string& directionStr) {
        int targetFloor = (directionStr == "Up") ? currentFloor + floorsToMove : currentFloor - floorsToMove;
        std::cout << "[Elevator] Moving from Floor " << currentFloor 
                  << " to Floor " << targetFloor << std::endl;

        while (currentFloor != targetFloor) {
            if (currentFloor < targetFloor) {
                currentFloor++;
                state = ElevatorState::MovingUp;
                std::cout << "[Elevator] Moving up: " << currentFloor << std::endl;
            } else {
                currentFloor--;
                state = ElevatorState::MovingDown;
                std::cout << "[Elevator] Moving down: " << currentFloor << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        doorOperations();
    }

    /*
    * Opens and closes the elevator doors at the current floor. The elevator
    * state is updated to DoorOpening, DoorOpen, and DoorClosing as the doors
    * open, remain open, and close, respectively. The elevator waits for 2
    * seconds between each state change.
    */
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

    /*
    * Processes the elevator event by moving the elevator to the pickup floor,
    * opening and closing the doors, and moving to the destination floor. The
    * elevator notifies the floor subsystem of its actions before and after
    * moving between floors.
    */
    void processRequest(const ElevatorEvent& item) {
        if (currentFloor != item.floor) {
            std::cout << "[Elevator] Moving to pickup floor " << item.floor << std::endl;
            moveToFloor(item.floor);
        } else {
            std::cout << "[Elevator] Already at pickup floor: " << currentFloor << std::endl;
        }
        floorNotifier.put(item);
        moveByFloors(item.floorsToMove, item.floorButton);
        floorNotifier.put(item);
    }

public:
    Elevator(Scheduler<Type>& sched, Scheduler<Type>& notifier)
        : scheduler(sched), floorNotifier(notifier), state(ElevatorState::Idle),
          direction(Direction::Idle), currentFloor(1) {}

    void operator()() {
        while (true) {
            std::cout << "[Elevator] Waiting for next task..." << std::endl;
            ElevatorEvent item = scheduler.get();
            std::cout << "[Elevator] Processing: " << item.display() << std::endl;
            processRequest(item);
            std::cout << "[Scheduler] Request completed\n";
        }
    }

    void moveToFloor(int targetFloor) {
        if (currentFloor == targetFloor) {
            std::cout << "[Elevator] Already at requested floor: " << currentFloor << std::endl;
            return;
        }
        while (currentFloor != targetFloor) {
            currentFloor += (currentFloor < targetFloor) ? 1 : -1;
            std::cout << "[Elevator] Passing floor: " << currentFloor << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        doorOperations();
    }
};

#endif // ELEVATOR_H
