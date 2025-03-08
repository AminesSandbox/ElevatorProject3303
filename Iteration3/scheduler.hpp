/*
 * SYSC 3303 B1-Group 1 
 * Created by Hasan Suriya
 * Reviewed by Abdullah Arid
 * 
 * This header file defines a class for the Scheduler subsystem. The 
 * Scheduler class schedules items, it includes methods to add an 
 * item to the queue and retrieve an item from the queue.
 * 
*/

#ifndef SCHEDULER_H
#define SCHEDULER_H

#define FLOORREADER 23
#define FLOORNOTIFIER 24
#define ELEVATOR_1 69
#define ELEVATOR_2 70

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <unistd.h>
#include <string>
#include <time.h>
#include "elevator_event.hpp"
#include "Datagram1.h"

// Enum for different scheduler states
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

    // Helper function to print state transitions
    void printStateChange(SchedulerState newState) {
        if (state != newState) {
            std::cout << "[Scheduler] State changed: " << stateToString(state) 
                      << " -> " << stateToString(newState) << std::endl;
            state = newState;
        }
    }

    // Convert state to string for printing
    std::string stateToString(SchedulerState s) const {
        switch (s) {
            case SchedulerState::BUSY: return "BUSY";
            case SchedulerState::IDLE: return "IDLE";
            default: return "UNKNOWN";
        }
    }
public:
    Scheduler(int PORT) : ClientSocket(PORT), ServerSocket(), state(SchedulerState::IDLE)  {}

    // Adds an item to the queue and notifies waiting threads.
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

    // Retrieves an item from the queue.
    Type get() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !queue.empty(); });
        Type item = queue.front();
        queue.pop();
        printStateChange(queue.empty() ? SchedulerState::IDLE : SchedulerState::BUSY);
        return item;
    }

    // Receives packet from sender
    std::vector<uint8_t> receiveClient() {
        std::vector<uint8_t> clientData(14);
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
        std::vector<uint8_t> clientData(14);
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
        if (static_cast<int>(data[0]) == 0 && static_cast<int>(data[1]) == 1) {
            //Packet Sturcture: 01HHMMSSsFF2FF (2 is up/down)
            int hourInt = static_cast<int>(data[2]) * 10 + static_cast<int>(data[3]);
            int minInt = static_cast<int>(data[4]) * 10 + static_cast<int>(data[5]);
            int secInt = static_cast<int>(data[6]) * 10 + static_cast<int>(data[7]);
            struct tm timestamp = {.tm_sec = secInt, .tm_min = minInt, .tm_hour = hourInt};
            int floor = static_cast<int>(data[9]) * 10 + static_cast<int>(data[10]);
            std::string floorButton = static_cast<int>(data[11]) == 1 ? "Up" : "Down";
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
        packet_data.push_back(item.floorButton.compare("Down") ? 1 : 0);
        packet_data.push_back(item.floorsToMove / 10 % 10);
        packet_data.push_back(item.floorsToMove % 10);
        /* std::cout << "Printing Packet: "; */
        /* printPacket(packet_data); */
        return packet_data;
    }

    // Returns the current state of the scheduler.
    SchedulerState getState() const {
        return state;
    }
 
};

void floorReader(Scheduler<ElevatorEvent>* scheduler);
void alertElevator(Scheduler<ElevatorEvent>* scheduler);

#endif // SCHEDULER_H

