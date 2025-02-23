#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "iteration2.cpp"
#include <thread>
#include "iostream"
#include <chrono>
#include <sstream>

TEST_CASE ("Test Input File") {
    Scheduler<ElevatorEvent> scheduler;
    Scheduler<ElevatorEvent> floorNotifier;
    std::ofstream testFile("elevator.txt");
    testFile << "14:05:15.0 2 Up 4\n";
    testFile << "14:07:30.5 6 Down 1\n";
    testFile << "14:12:45.2 5 Up 10\n";
    testFile << "14:20:00.0 15 Down 2\n";
    testFile << "15:00:15.7 13 Down 6\n";
    testFile.close();
    
    Floor<ElevatorEvent> floorReader("elevator.txt", scheduler, floorNotifier);
    std::thread floorThread(std::ref(floorReader));
    floorThread.join();
    
    ElevatorEvent event = scheduler.get();
    CHECK(event.floor == 2);
    CHECK(event.floorButton == "Up");
    CHECK(event.floorsToMove == 4);
}


// Test ElevatorEvent structure
TEST_CASE("ElevatorEvent displays correct message") {
    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 2, "Up", 4);
    CHECK(event.display() == "[Floor Subsystem] Request sent: Floor 2 going Up by 4 floors");
}

// Test Scheduler functionality
TEST_CASE("Scheduler handles put and get correctly") {
    Scheduler<ElevatorEvent> scheduler;
    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 6, "Down", 1);
    
    scheduler.put(event);
    ElevatorEvent receivedEvent = scheduler.get();
    CHECK(receivedEvent.floor == 6);
    CHECK(receivedEvent.floorButton == "Down");
    CHECK(receivedEvent.floorsToMove == 1);
}

// Test Elevator movement
TEST_CASE("Elevator moves to correct floor") {
    Scheduler<ElevatorEvent> scheduler;
    Scheduler<ElevatorEvent> floorNotifier;
    Elevator<ElevatorEvent> elevator(scheduler, floorNotifier);
    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 5, "Up", 10);
    
    std::thread elevatorThread([&]() { elevator.moveToFloor(5); });
    elevatorThread.join();
    
    CHECK(elevator.getCurrentFloor() == 5);
}

// Test Elevator states
TEST_CASE("Elevator transitions through all states correctly") {
    Scheduler<ElevatorEvent> scheduler;
    Scheduler<ElevatorEvent> floorNotifier;
    Elevator<ElevatorEvent> elevator(scheduler, floorNotifier);

    std::thread elevatorThread([&]() {
        elevator.moveToFloor(3);  // Start moving elevator to floor 3
    });

    // Now check the states while the elevator is moving
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait a bit for elevator to start moving
    CHECK(elevator.getState() == ElevatorState::MovingUp);

    std::this_thread::sleep_for(std::chrono::seconds(5)); 
    CHECK(elevator.getState() == ElevatorState::DoorOpen); // Should be door open when reached floor

    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for door closing
    CHECK(elevator.getState() == ElevatorState::DoorClosing);

    std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait a bit to check final state
    CHECK(elevator.getState() == ElevatorState::Idle); 

    elevatorThread.join();  
}
