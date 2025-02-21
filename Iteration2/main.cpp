/*
 * SYSC 3303 B1-Group 1 
 * Created by Hasan Suriya
 * Reviewed by Abdullah Arid
 * 
 * This file defines the main function for the elevator simulation. It creates
 * a scheduler for handling elevator events, a scheduler for notifying floors,
 * a floor reader, and an elevator. It then creates threads for the floor and
 * elevator and waits for them to finish.
 * 
*/

#include "elevator.hpp"
#include "floor.hpp"
#include "scheduler.hpp"
#include "elevator_event.hpp"

int main() {
    // Create a scheduler for handling elevator events
    Scheduler<ElevatorEvent> scheduler;
    
    // Create a scheduler for notifying floors
    Scheduler<ElevatorEvent> floorNotifier;

    // Initialize the floor reader with the input file and schedulers
    Floor<ElevatorEvent> floorReader("elevator.txt", scheduler, floorNotifier);
    
    // Create a thread to handle floor events
    std::thread floorThread(std::ref(floorReader));

    // Initialize the elevator with the schedulers
    Elevator<ElevatorEvent> elevator(scheduler, floorNotifier);
    
    // Create a thread to handle elevator events
    std::thread elevatorThread(std::ref(elevator));

    // Wait for the elevator thread to finish
    elevatorThread.join();
    
    // Wait for the floor thread to finish
    floorThread.join();

    return 0;
}
