#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <iostream>
#include <thread>
#include <chrono>
#include "scheduler.hpp"
#include "elevator_event.hpp"

enum class ElevatorState { Idle, MovingUp, MovingDown, DoorOpening, DoorOpen, DoorClosing, MinorFault, MajorFault };
enum class Direction { Up, Down, Idle };
struct DisplayEvent {
    int elevatorID;
    int floor;
    std::string direction;
    std::string status;
};

template <typename Type>
class Elevator {
private:
    ElevatorState state;
    Direction direction;
    int currentFloor;
    DatagramSocket receiveSocket;
    DatagramSocket sendSocket;
    int id;
    const int MAX_CAPACITY = 4;


    bool moveByFloors(int floorsToMove, const std::string& directionStr, int passengers) {
        int targetFloor = (directionStr == "Up") ? currentFloor + floorsToMove : currentFloor - floorsToMove;
        std::cout << "[Elevator" << id << "] Moving from Floor " << currentFloor 
                  << " to Floor " << targetFloor << std::endl
                  << "[Elevator" << id << "] Has Passengers: " << passengers << std::endl;

        auto startTime = std::chrono::steady_clock::now();     

        while (currentFloor != targetFloor) {
            if (currentFloor < targetFloor) {
                currentFloor++;
                state = ElevatorState::MovingUp;
                sendDisplayUpdate();
                std::cout << "[Elevator" << id << "] Moving up: " << currentFloor << std::endl;
            } else {
                currentFloor--;
                state = ElevatorState::MovingDown;
                sendDisplayUpdate();
                std::cout << "[Elevator" << id << "] Moving down: " << currentFloor << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto elapsedTime = std::chrono::steady_clock::now() - startTime;
            auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

        }
        return true;
    }

    bool doorOperations() {
        state = ElevatorState::DoorOpening;
        sendDisplayUpdate();

        std::cout << "[Elevator" << id << "] Doors opening at floor: " << currentFloor << std::endl;

        auto startTime = std::chrono::steady_clock::now(); // Start timer

        std::this_thread::sleep_for(std::chrono::seconds(1));

        state = ElevatorState::DoorOpen;
        sendDisplayUpdate();
        std::cout << "[Elevator" << id << "] Boarding at floor: " << currentFloor << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        state = ElevatorState::DoorClosing;
        sendDisplayUpdate();
        std::cout << "[Elevator" << id << "] Doors closing at floor: " << currentFloor << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        state = ElevatorState::Idle;
        sendDisplayUpdate();
        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count();

        return true;
    }

    void handleFloorFault() {
        state = ElevatorState::MajorFault;
        std::cout << "[Elevator" << id << "] Floor Timer Fault: Elevator is stuck between floors!" << std::endl;
        sendDisplayUpdate();
        throw std::runtime_error("Major fault in elevator. Shutting down this thread.");
    }

    void handleDoorFault() {
        state = ElevatorState::MinorFault;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "[Elevator" << id << "] Door Timer Fault: Door is stuck!" << std::endl;
        sendDisplayUpdate();
        recoverDoor();
    }

    void recoverDoor() {
        std::cout << "[Elevator" << id << "] Attempting to recover door..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        state = ElevatorState::Idle;
        sendDisplayUpdate();
        std::cout << "[Elevator" << id << "] Door recovered successfully!" << std::endl;
    }

public:
    Elevator(int PORT, int id)
        : state(ElevatorState::Idle), direction(Direction::Idle), 
        currentFloor(1), receiveSocket(PORT), id(id) {}

    int getCurrentFloor() const { return currentFloor; }

    ElevatorState getState() const { return state; }

    void sendDisplayUpdate() {
        std::vector<uint8_t> data;
        data.push_back(id); // elevator ID
        data.push_back(currentFloor);
        
        if (direction == Direction::Up) data.push_back(1);
        else if (direction == Direction::Down) data.push_back(2);
        else data.push_back(0);
    
        data.push_back(static_cast<int>(state)); // ElevatorState enum to int
    
        DatagramPacket pkt(data, data.size(), InetAddress::getLocalHost(), DISPLAY_PORT);
        sendSocket.send(pkt);
    }
     
   void processRequest(const ElevatorEvent& item) {
        if (item.passengers > MAX_CAPACITY) {
            std::cout << "[Elevator" << id << "] Over capacity (" << item.passengers 
            << " > " << MAX_CAPACITY << "), cannot board!\n";

            std::vector<uint8_t> data = createData(item);
            std::this_thread::sleep_for(std::chrono::seconds(1)); 
            sendPacket(data, data.size(), InetAddress::getLocalHost(), FLOORREADER);  
            
            state = ElevatorState::Idle;
            sendDisplayUpdate();
            return;  
        }
        if (item.fault == "Major") {
            handleFloorFault();
        } 
        if (currentFloor != item.floor) {
            std::cout << "[Elevator" << id << "] Moving to pickup floor " << item.floor << " with " << item.passengers << std::endl;
            moveToFloor(item.floor);
            sendDisplayUpdate();
            if (item.fault == "Minor") {
                handleDoorFault();
            }
            doorOperations();
        } else {
            std::cout << "[Elevator" << id << "] Already at pickup floor: " << currentFloor << std::endl;
        }

        std::vector<uint8_t> packet_data = createData(item);
        sendPacket(packet_data, packet_data.size(), InetAddress::getLocalHost(), FLOORNOTIFIER);

        if (!moveByFloors(item.floorsToMove, item.floorButton, item.passengers)) {
            sendDisplayUpdate();
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
                try {
                    processRequest(item); 
                } catch (const std::runtime_error& e) {
                    std::cerr << "[Elevator" << id << "] Critical error: " << e.what() << std::endl;
                    std::cerr << "[Elevator" << id << "] Going out of service." << std::endl;
                    break;  // clean shutdown of thread
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
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
 
            currentFloor += (currentFloor < targetFloor) ? 1 : -1;
            std::cout << "[Elevator" << id << "] Passing floor: " << currentFloor << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::vector<uint8_t> receivePacket() {

        std::vector<uint8_t> packetData(17);
        DatagramPacket schedulerPacket(packetData, packetData.size());

        try {
            receiveSocket.receive(schedulerPacket);
        } catch (const std::runtime_error& e ) {
            std::cerr << e.what() << std::endl;
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

    Type processData(std::vector<uint8_t> data) {
        return ElevatorEvent::parseFromPacket(data);
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
        } else {
            packet_data.push_back(0xF); // unknown state
        }
        packet_data.push_back(item.passengers / 10 % 10);
        packet_data.push_back(item.passengers % 10);
        packet_data.push_back(item.floorsToMove / 10 % 10);
        packet_data.push_back(item.floorsToMove % 10);
        if (item.fault == "Minor") {
            packet_data.push_back(1); // Minor fault
        } else if (item.fault == "Major") {
            packet_data.push_back(2); // Major fault
        } else {
            packet_data.push_back(0); // No fault
        }
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
        sendDisplayUpdate();
    }

};


#endif // ELEVATOR_H
