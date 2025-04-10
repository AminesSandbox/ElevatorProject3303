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
    ElevatorEvent event(timestamp, 2, "Up", 4, 0, "None");
    CHECK(event.display() == "\n [Floor Subsystem] Request sent:\n  Floor: 2\n  Direction: Up\n  Floors to move: 4\n  Passengers: 0\n  Fault: None");
}

// Test Scheduler functionality
TEST_CASE("Scheduler handles put and get correctly") {
    Scheduler<ElevatorEvent> scheduler(23);
    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 6, "Down", 1, 0, "None");
    
    scheduler.put(event);
    ElevatorEvent receivedEvent = scheduler.get();
    CHECK(receivedEvent.floor == 6);
    CHECK(receivedEvent.floorButton == "Down");
    CHECK(receivedEvent.floorsToMove == 1);
}

// Test Scheduler Algorithm
TEST_CASE("Scheduler alternates elevators correctly in alertElevator") {
    Scheduler<ElevatorEvent> scheduler(23);

    struct tm timestamp = {};
    ElevatorEvent event1(timestamp, 2, "Up", 3, 0, "None");
    ElevatorEvent event2(timestamp, 4, "Down", 2, 0, "None");
    ElevatorEvent event3(timestamp, 6, "Up", 1, 0, "None");
    ElevatorEvent event4(timestamp, 8, "Down", 5, 0, "None");

    scheduler.put(event1);
    scheduler.put(event2);
    scheduler.put(event3);
    scheduler.put(event4);

    std::vector<int> assignedElevators;
    std::vector<uint8_t> data;

    int i = 0;
    while (i < 4) {  
        ElevatorEvent event = scheduler.get(); 
        data = scheduler.createData(event);  
        
        int result = 0;
        if (i % 2 == 0) {
            result = scheduler.sendPacket(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_1);
            assignedElevators.push_back(ELEVATOR_1);
        } else {
            result = scheduler.sendPacket(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_2);
            assignedElevators.push_back(ELEVATOR_2);
        }
        i++;
    }

    CHECK(assignedElevators[0] == ELEVATOR_1);
    CHECK(assignedElevators[1] == ELEVATOR_2);
    CHECK(assignedElevators[2] == ELEVATOR_1);
    CHECK(assignedElevators[3] == ELEVATOR_2);
}


// Test Elevator movement
TEST_CASE("Elevator moves to correct floor") {
    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    Elevator<ElevatorEvent> elevator(69, 1);

    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 5, "Up", 10, 0, "None");
    
    std::thread elevatorThread([&]() { elevator.moveToFloor(5); });
    elevatorThread.join();
    
    CHECK(elevator.getCurrentFloor() == 5);
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
    ElevatorEvent event(timestamp, 6, "Down", 1, 0, "None");

    std::thread elevatorThread([&]() {
        elevator.processRequest(event);  // Start moving elevator to floor 6
    });

    std::vector<ElevatorState> expectedStates = {
        ElevatorState::DoorOpening, ElevatorState::DoorOpen,
        ElevatorState::DoorClosing,ElevatorState::MovingDown, 
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

TEST_CASE("Test if elevator enters Minor Fault state") {

    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    Elevator<ElevatorEvent> elevator(69, 1);

    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 7, "Up", 6, 0, "Minor");

    std::thread elevatorThread([&]() {
        elevator.processRequest(event);  // Start moving elevator to floor 6
    });

    std::vector<ElevatorState> expectedStates = {
        ElevatorState::MinorFault,
        ElevatorState::DoorOpening, ElevatorState::DoorOpen,
        ElevatorState::DoorClosing,ElevatorState::MovingUp, 
        ElevatorState::DoorOpening, ElevatorState::DoorOpen, 
        ElevatorState::DoorClosing, ElevatorState::Idle
    };

    std::vector<ElevatorState> actualStates;

    ElevatorState lastState = ElevatorState::Idle;
    for (int i = 0; i < 45; i++) {
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

TEST_CASE ("Testing Capacity Limit (Overflow (5ppl))") {
    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    Elevator<ElevatorEvent> elevator(69, 1);

    struct tm timestamp = {};
    ElevatorEvent event(timestamp, 7, "Up", 6, 7, "None");

    std::thread elevatorThread([&]() {
        elevator.processRequest(event); 
    });

    CHECK(elevator.getState() == ElevatorState::Idle);

    elevatorThread.join();  
}