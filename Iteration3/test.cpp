#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "scheduler.hpp"

#include "elevator_event.hpp"

#include "elevator.hpp"

#include "floor.hpp"

#include <thread>
#include "iostream"
#include <chrono>
#include <sstream>

// Test ElevatorEvent structure
TEST_CASE("ElevatorEvent displays correct message") {
    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 2, "Up", 4);
    CHECK(event.display() == "[Floor Subsystem] Request sent: Floor 2 going Up by 4 floors");
}

// Test Scheduler functionality
TEST_CASE("Scheduler handles put and get correctly") {
    Scheduler<ElevatorEvent> scheduler(23);
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
    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    Elevator<ElevatorEvent> elevator(69, 1);

    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 5, "Up", 10);
    
    std::thread elevatorThread([&]() { elevator.moveToFloor(5); });
    elevatorThread.join();
    
    CHECK(elevator.getCurrentFloor() == 5);
}

// Test Elevator states (Same Floor)
TEST_CASE("Elevator transitions through all states correctly when idle") {
    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    Elevator<ElevatorEvent> elevator(69, 1);

    std::thread elevatorThread([&]() {
        elevator.moveToFloor(3);  // Start moving elevator to floor 3
    });

    std::vector<ElevatorState> expectedStates = {
        ElevatorState::DoorOpening, ElevatorState::DoorOpen,
        ElevatorState::DoorClosing, ElevatorState::Idle,
    };

    std::vector<ElevatorState> actualStates;

    ElevatorState lastState = ElevatorState::Idle;
    for (int i = 0; i < 12; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        //collect all states elevator goes through
        ElevatorState state = elevator.getState();
        if (state != lastState) {
            actualStates.push_back(state);
            lastState = state;
        }
    }

    for (size_t i = 0; i < expectedStates.size(); ++i) {
        CHECK(actualStates[i] == expectedStates[i]);
    }


    elevatorThread.join();  
}

// Test Elevator states (Moving Floors)
TEST_CASE("Elevator transitions through all states correctly when moving") {
    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    Elevator<ElevatorEvent> elevator(69, 1);
    
    struct tm timestamp = {};
    timestamp.tm_year = 2023 - 1900; 
    timestamp.tm_mon = 9;            
    timestamp.tm_mday = 25;      
    timestamp.tm_hour = 10;    
    timestamp.tm_min = 30;  
    timestamp.tm_sec = 0; 
    ElevatorEvent event(timestamp, 6, "Down", 1);

    std::thread elevatorThread([&]() {
        elevator.processRequest(event);  // Start moving elevator to floor 6
    });

    std::vector<ElevatorState> expectedStates = {
        ElevatorState::DoorOpening, ElevatorState::DoorOpen,
        ElevatorState::DoorClosing, ElevatorState::MovingDown,
        ElevatorState::DoorOpening, ElevatorState::DoorOpen,
        ElevatorState::DoorClosing, ElevatorState::Idle
    };

    std::vector<ElevatorState> actualStates;

    ElevatorState lastState = ElevatorState::Idle;
    for (int i = 0; i < 20; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        //collect all states elevator goes through
        ElevatorState state = elevator.getState();
        if (state != lastState) {
            actualStates.push_back(state);
            lastState = state;
        }
    }
    std::cout << "Actual states: ";
    for (const auto& state : actualStates) {
        std::cout << static_cast<int>(state) << " ";
    }
    std::cout << std::endl;

    for (size_t i = 0; i < expectedStates.size(); ++i) {
        CHECK(actualStates[i] == expectedStates[i]);
    }

    elevatorThread.join();  
}