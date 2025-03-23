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
#define FLOORNOTIFIER 24

#include <iostream>
#include <thread>
#include <chrono>
#include "scheduler.hpp"
#include "elevator_event.hpp"

enum class ElevatorState { Idle, MovingUp, MovingDown, DoorOpening, DoorOpen, DoorClosing, MinorFault, MajorFault };
enum class Direction { Up, Down, Idle };

template <typename Type>
class Elevator {
private:
    ElevatorState state;
    Direction direction;
    int currentFloor;
    DatagramSocket receiveSocket;
    DatagramSocket sendSocket;
    int id;


    /*
    * Moves the elevator to the target floor by incrementing or decrementing
    * the current floor until it reaches the target floor. The elevator state
    * is updated to MovingUp or MovingDown as it moves between floors.
    */
    bool moveByFloors(int floorsToMove, const std::string& directionStr) {
        int targetFloor = (directionStr == "Up") ? currentFloor + floorsToMove : currentFloor - floorsToMove;
        std::cout << "[Elevator" << id << "] Moving from Floor " << currentFloor 
                  << " to Floor " << targetFloor << std::endl;

    auto startTime = std::chrono::steady_clock::now(); // Start timer    

    while (currentFloor != targetFloor) {
        //if (isFault()) {
            //return false;
        //}

        if (currentFloor < targetFloor) {
            currentFloor++;
            state = ElevatorState::MovingUp;
            std::cout << "[Elevator" << id << "] Moving up: " << currentFloor << std::endl;
        } else {
            currentFloor--;
            state = ElevatorState::MovingDown;
            std::cout << "[Elevator" << id << "] Moving down: " << currentFloor << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

        if (elapsedSeconds > 15) { // Major Fault
            handleFloorFault();
            return false;
        }
    }
    return true;
}

    /*
    * Opens and closes the elevator doors at the current floor. The elevator
    * state is updated to DoorOpening, DoorOpen, and DoorClosing as the doors
    * open, remain open, and close, respectively. The elevator waits for 2
    * seconds between each state change.
    */
   bool doorOperations() {
    state = ElevatorState::DoorOpening;
    std::cout << "[Elevator" << id << "] Doors opening at floor: " << currentFloor << std::endl;

    auto startTime = std::chrono::steady_clock::now(); // Start timer

    std::this_thread::sleep_for(std::chrono::seconds(2));

    //if (isFault()) {
        //return false;
    //}

    state = ElevatorState::DoorOpen;
    std::cout << "[Elevator" << id << "] Boarding at floor: " << currentFloor << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    //if (isFault()) {
        //return false;
    //}

    state = ElevatorState::DoorClosing;
    std::cout << "[Elevator" << id << "] Doors closing at floor: " << currentFloor << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    state = ElevatorState::Idle;

    auto elapsedTime = std::chrono::steady_clock::now() - startTime;
    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

    if (elapsedSeconds > 5) { // Minor Fault
        handleDoorFault();
        return false;
    }

    //if (isFault()) {
        //return false;
    //}
    return true;
}

    /*
    Returns whether the elvator is in a fault state
    */
    bool isFault() {
        if (state == ElevatorState::MinorFault || state == ElevatorState::MajorFault) {
            std::cout << "[Elevator" << id << "] Elevator is in a fault state: " << std::endl;
            return true;
        }
        return false;
    }


    void handleFloorFault() {
        state = ElevatorState::MajorFault;
        std::cout << "[Elevator" << id << "] Floor Timer Fault: Elevator is stuck between floors!" << std::endl;
        shutdown();
    }

    void handleDoorFault() {
        state = ElevatorState::MinorFault;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "[Elevator" << id << "] Door Timer Fault: Door is stuck!" << std::endl;
        recoverDoor();
    }

    void shutdown() {
        std::cout << "[Elevator" << id << "] Shutting down due to Major Fault!" << std::endl;
        // Stop all operations
        state = ElevatorState::MajorFault;
    }

    void recoverDoor() {
        std::cout << "[Elevator" << id << "] Attempting to recover door..." << std::endl;
        // Retry door operations
        state = ElevatorState::Idle;
        std::cout << "[Elevator" << id << "] Door recovered successfully!" << std::endl;
    }

public:
    Elevator(int PORT, int id)
        : state(ElevatorState::Idle), direction(Direction::Idle), 
        currentFloor(1), receiveSocket(PORT), id(id) {}

    int getCurrentFloor() const { return currentFloor; }

    ElevatorState getState() const { return state; }
    
    /*
    * Processes the elevator event by moving the elevator to the pickup floor,
    * opening and closing the doors, and moving to the destination floor. The
    * elevator notifies the floor subsystem of its actions before and after
    * moving between floors.
    */
   void processRequest(const ElevatorEvent& item) {

        if (currentFloor != item.floor) {
            std::cout << "[Elevator" << id << "] Moving to pickup floor " << item.floor << std::endl;
            moveToFloor(item.floor);
            doorOperations();
        } else {
            std::cout << "[Elevator" << id << "] Already at pickup floor: " << currentFloor << std::endl;
        }

        //if (isFault()) {
            //return;
        //}

        std::vector<uint8_t> packet_data = createData(item);
        sendPacket(packet_data, packet_data.size(), InetAddress::getLocalHost(), FLOORNOTIFIER);

        if (!moveByFloors(item.floorsToMove, item.floorButton)){
            return;
        }
        if (!doorOperations()){
            return;
        }

        sendPacket(packet_data, packet_data.size(), InetAddress::getLocalHost(), FLOORNOTIFIER);
    }

    void operator()() {
        while (true) {
            std::cout << "[Elevator" << id << "] Waiting for next task..." << std::endl;
            std::vector<uint8_t> data = receivePacket();
            if (static_cast<int>(data[0]) == 0 && static_cast<int>(data[1]) == 1 && ElevatorState::Idle == state) {

                ElevatorEvent item = processData(data);
                std::cout << "[Elevator" << id << "] Processing: " << item.display() << std::endl;
                processRequest(item); // will exit early if in a fault state

                std::this_thread::sleep_for(std::chrono::seconds(1));

                if(isFault()) {
                    std::cout << "[Elevator" << id << "] Elevator is in a fault state: " << std::endl;
                    std::cout << "[Elevator" << id << "] state: ";
                    std::cout << static_cast<int>(state) << std::endl;
                    std::cout << "[Elevator" << id << "] Elevator Awaiting Scheduler response. " << std::endl;
                } else {
                    std::vector<uint8_t> success_msg(14,1);
                    sendPacket(success_msg, success_msg.size(), InetAddress::getLocalHost(), FLOORNOTIFIER);
                }
            }else{
                std::cout << "[Elevator" << id << "] Unable to process request, Elevator not in IDLE state" << std::endl;
            }
        }
    }

    void moveToFloor(int targetFloor) {
        if (currentFloor == targetFloor) {
            std::cout << "[Elevator" << id << "] Already at requested floor: " << currentFloor << std::endl;
            return;
        }
        while (currentFloor != targetFloor) {
            //if (isFault()) {
                //return;
            //}

            currentFloor += (currentFloor < targetFloor) ? 1 : -1;
            std::cout << "[Elevator" << id << "] Passing floor: " << currentFloor << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::vector<uint8_t> receivePacket() {

        std::vector<uint8_t> packetData(14);
        DatagramPacket schedulerPacket(packetData, packetData.size());

        try {
            // std::cout << "Server: Waiting for Packet." << std::endl;
            receiveSocket.receive(schedulerPacket);
        } catch (const std::runtime_error& e ) {
            std::cout << "IO Exception: likely:"
                  << "Receive Socket Timed Out." << std::endl << e.what() << std::endl;
            exit(1);
        }

        return packetData;
    }

    void printPacket(std::vector<uint8_t> packet_data) {
        for (char i: packet_data)
            if (i <= 0x10) {
                std::cout << std::hex << static_cast<int>(i);
            }
            else {
            std::cout << i;
            }
        std::cout << std::endl;
    }

    Type processData(std::vector<uint8_t> data){
        if (static_cast<int>(data[0]) == 0 && static_cast<int>(data[1]) == 1) {
            //Packet Sturcture: 01HHMMSSsFF2FF (2 is up/down)
            int hourInt = static_cast<int>(data[2]) * 10 + static_cast<int>(data[3]);
            int minInt = static_cast<int>(data[4]) * 10 + static_cast<int>(data[5]);
            int secInt = static_cast<int>(data[6]) * 10 + static_cast<int>(data[7]);
            struct tm timestamp = {.tm_sec = secInt, .tm_min = minInt, .tm_hour = hourInt};
            int floor = static_cast<int>(data[9]) * 10 + static_cast<int>(data[10]);
            
            std::string floorButton;
            
            if (data[11] == 1) {
                floorButton = "Up";  // Elevator moving Up
            } else if (data[11] == 0) {
                floorButton = "Down"; // Elevator moving Down
            } else if (data[11] == 2) {
                floorButton = "MinorError"; // minor error state
            } else if (data[11] == 3) {
                floorButton = "MajorError"; // major error state
            } else {
                floorButton = "Error - Unknown State"; // unknown state
            }


            int floorsToMove = static_cast<int>(data[12]) * 10 + static_cast<int>(data[13]);
            /* std::cout << floor << std::endl; */
            /* std::cout << floorButton << std::endl; */
            /* std::cout << floorsToMove << std::endl; */
            return Type(timestamp, floor, floorButton, floorsToMove);
        }
        throw std::runtime_error("Unable to process packet");
    }

    std::vector<uint8_t> createData (Type item) {

        int hour = item.timestamp.tm_hour;
        int min = item.timestamp.tm_min;
        int sec = item.timestamp.tm_sec;
        int msec = 0;

        std::vector<uint8_t> packet_data;
        packet_data.push_back(0x0);
        packet_data.push_back(0x1);

        packet_data.push_back(hour / 10 % 10);
        packet_data.push_back(hour % 10);
        packet_data.push_back(min / 10 % 10);
        packet_data.push_back(min % 10);
        packet_data.push_back(sec / 10 % 10);
        packet_data.push_back(sec % 10);
        packet_data.push_back(msec % 10);
        packet_data.push_back(item.floor / 10 % 10);
        packet_data.push_back(item.floor % 10);
        if (item.floorButton == "Up") {
            packet_data.push_back(1);  // Elevator moving Up
        } else if (item.floorButton == "Down") {
            packet_data.push_back(0); // Elevator moving Down
        } else if (item.floorButton == "MinorFault") {
            packet_data.push_back(2); // minor error state
        } else if (item.floorButton == "MajorFault") {
            packet_data.push_back(3); // major error state
        } else {
            packet_data.push_back(0xF); // unknown state
        }
        packet_data.push_back(item.floorsToMove / 10 % 10);
        packet_data.push_back(item.floorsToMove % 10);
        /* std::cout << "Printing Packet: "; */
        /* printPacket(packet_data); */
        return packet_data;
    }

    void sendPacket(std::vector<uint8_t> data, int size, in_addr_t address, int port) {
        DatagramPacket sendPacket(data, size, address, port);

        /* std::this_thread::sleep_for( std::chrono::seconds(2)); */
        /* printPacket(data, sendPacket, 1); */

        try {
            sendSocket.send(sendPacket);
        } catch ( const std::runtime_error& e ) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        return;
    }

    void setState(ElevatorState state) {
        this->state = state;
    }

};


#endif // ELEVATOR_H
