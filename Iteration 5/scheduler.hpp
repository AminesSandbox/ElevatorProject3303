#ifndef SCHEDULER_H
#define SCHEDULER_H

#define FLOORREADER 23
#define FLOORNOTIFIER 24
#define ELEVATOR_1 69
#define ELEVATOR_2 70
#define ELEVATOR_3 471
#define ELEVATOR_4 472
#define DISPLAY_CONSOLE 75
#define DISPLAY_PORT 99

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <unistd.h>
#include <string>
#include <time.h>
#include "elevator_event.hpp"
#include "Datagram1.h"

enum class SchedulerState {
    BUSY,
    IDLE
};

template <typename Type>
class Scheduler {
private:
    std::queue<Type> queue;
    std::mutex mtx;
    std::condition_variable cv;
    DatagramSocket ServerSocket;
    DatagramSocket ClientSocket;
    SchedulerState state;

    void printStateChange(SchedulerState newState) {
        if (state != newState) {
            std::cout << "[Scheduler] State changed: " << stateToString(state) 
                      << " -> " << stateToString(newState) << std::endl;
            state = newState;
        }
    }

    std::string stateToString(SchedulerState s) const {
        switch (s) {
            case SchedulerState::BUSY: return "BUSY";
            case SchedulerState::IDLE: return "IDLE";
            default: return "UNKNOWN";
        }
    }
public:
    Scheduler(int PORT) : ClientSocket(PORT), ServerSocket(), state(SchedulerState::IDLE)  {}

    void put(Type item) {
        std::unique_lock<std::mutex> lock(mtx);
        bool wasEmpty = queue.empty(); // Check if queue was empty before adding
        if (wasEmpty) {
            printStateChange(SchedulerState::BUSY); // Change state only if it was previously idle
        }
        std::cout << "[Scheduler] Assign " << item.display() << " to Elevator" << std::endl;
        queue.push(item);
        cv.notify_all();
    }

    Type get() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !queue.empty(); });
        Type item = queue.front();
        queue.pop();
        printStateChange(queue.empty() ? SchedulerState::IDLE : SchedulerState::BUSY);
        return item;
    }

    std::vector<uint8_t> receiveClient() {
        std::vector<uint8_t> clientData(17);
        DatagramPacket clientPacket(clientData, clientData.size());
        /* std::cout << "Server: Waiting for Packet." << std::endl; */

        try {
            /* std::cout << "Waiting..." << std::endl; // so we know we're waiting */
            ClientSocket.receive(clientPacket);
        } catch (const std::runtime_error& e ) {
            std::cout << "IO Exception: likely:"
                  << "Receive Socket Timed Out." << std::endl << e.what() << std::endl;
            exit(1);
        }

        /* printPacket(clientData); */
        return clientData;
    }

    DatagramPacket receiveAndStore() {
        std::vector<uint8_t> clientData(17);
        DatagramPacket clientPacket(clientData, clientData.size());
        /* std::cout << "Server: Waiting for Packet." << std::endl; */

        try {
            /* std::cout << "Waiting..." << std::endl; // so we know we're waiting */
            ClientSocket.receive(clientPacket);
        } catch (const std::runtime_error& e ) {
            std::cout << "IO Exception: likely:"
                  << "Receive Socket Timed Out." << std::endl << e.what() << std::endl;
            exit(1);
        }

        return clientPacket;
    }

    // Sends Packet
    int sendPacket(std::vector<uint8_t> data, int size, in_addr_t address, int port) {
        DatagramPacket sendPacket(data, size, address, port);
        int result = -1;
        try {
                result = ServerSocket.send(sendPacket);
        } catch ( const std::runtime_error& e ) {
            std::cerr << e.what() << std::endl;
            exit(1);
        }

        return result;
    }

    // Prints contents of packet
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

    //Processes packet into Type
    Type processData(std::vector<uint8_t> data){
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
        packet_data.push_back(item.floorButton.compare("Down") ? 1 : 0);
        packet_data.push_back(item.floorsToMove / 10 % 10);
        packet_data.push_back(item.floorsToMove % 10);
        packet_data.push_back(item.passengers / 10 % 10);
        packet_data.push_back(item.passengers % 10);
        if (item.fault.compare("Minor") == 0) {
            packet_data.push_back(1);  // Minor fault
        } else if (item.fault.compare("Major") == 0) {
            packet_data.push_back(2);  // Major fault
        } else {
            packet_data.push_back(0);  // No fault
        }


        /* std::cout << "Printing Packet: "; */
        /* printPacket(packet_data); */
        return packet_data;
    }

    // Returns the current state of the scheduler.
    SchedulerState getState() const {
        return state;
    }
 
};

void floorReader(Scheduler<ElevatorEvent>* scheduler) {
    while (true) {
        std::vector<uint8_t> packet = scheduler->receiveClient();
        ElevatorEvent event = scheduler->processData(packet);
        scheduler->put(event);
    }
}

void alertElevator(Scheduler<ElevatorEvent>* scheduler) {
    int i = 0;
    while(true){
        ElevatorEvent event = scheduler->get();
        std::vector<uint8_t> data = scheduler->createData(event);
        int targetPort= 0;

        switch (i % 4) {
            case 0: targetPort = ELEVATOR_1; break;
            case 1: targetPort = ELEVATOR_2; break;
            case 2: targetPort = ELEVATOR_3; break;
            case 3: targetPort = ELEVATOR_4; break;
        }
        scheduler->sendPacket(data, data.size(), InetAddress::getLocalHost(), targetPort);        
        i++;
    }
}


#endif // SCHEDULER_H

