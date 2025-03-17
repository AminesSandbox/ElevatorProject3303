/*
 * SYSC 3303 B1-Group 1 
 * Created by Hasan Suriya
 * Reviewed by Abdullah Arid
 * 
 * 
 * This header file defines a class for the Floor subsystem. The
 * Floor class reads floor requests from a file and sends them to the
 * scheduler for processing. It includes methods to process the floor
 * requests and send them to the scheduler.
 * 
*/

#ifndef FLOOR_H
#define FLOOR_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "scheduler.hpp"
#include "elevator_event.hpp"
/* #include "Datagram1.h" */

#define SCHEDULER 23
#define ELEVATOR 69

template <typename Type>
class Floor {
private:
    std::string filename;
    DatagramSocket sendSocket;

public:
    Floor(const std::string& file)
        : filename(file), sendSocket() {}

    std::vector<uint8_t> createData
    (std::string timeStr, std::string floorButton, int floor, int floorsToMove) {
        
        std::vector<uint8_t> packet_data;
        packet_data.push_back(0x0);
        packet_data.push_back(0x1);
        
        int hour = std::stoi(&timeStr[0]);
        int min = std::stoi(&timeStr[3]);
        int sec = std::stoi(&timeStr[6]);
        int msec = std::stoi(&timeStr[9]);
        packet_data.push_back(hour / 10 % 10);
        packet_data.push_back(hour % 10);
        packet_data.push_back(min / 10 % 10);
        packet_data.push_back(min % 10);
        packet_data.push_back(sec / 10 % 10);
        packet_data.push_back(sec % 10);
        packet_data.push_back(msec % 10);
        packet_data.push_back(floor / 10 % 10);
        packet_data.push_back(floor % 10);

        if (floorButton == "Up") {
            packet_data.push_back(0x1);  // Elevator moving Up
        } else if (floorButton == "Down") {
            packet_data.push_back(0x0); // Elevator moving Down
        } else if (floorButton == "MinorFault") {
            packet_data.push_back(0x2); // minor error state
        } else if (floorButton == "MajorFault") {
            packet_data.push_back(0x3); // major error state
        } else {
            packet_data.push_back(0xF); // unknown state
        }

        packet_data.push_back(floorButton.compare("Down") ? 1 : 0);
        packet_data.push_back(floorsToMove / 10 % 10);
        packet_data.push_back(floorsToMove % 10);
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

    void operator()() {
        std::ifstream file(filename); 
        if (!file) { 
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        /* std::cout << "[Scheduler] Request input from the Floor Subsystem" << std::endl; */
        std::string line;
        while (std::getline(file, line)) { 
            std::istringstream iss(line);
            std::string timeStr, floorButton;
            int floor, floorsToMove;

            if (!(iss >> timeStr >> floor >> floorButton >> floorsToMove)) {
                std::cerr << "Incorrect line format: " << line << std::endl;
                continue;
            }

            std::vector<uint8_t> packetInfo = createData(timeStr, floorButton, floor, floorsToMove);
            /* std::cout << timeStr << ": " << floor << ": " << floorButton << ": " << floorsToMove << std::endl; */
            /* printPacket(packetInfo); */

            sendPacket(packetInfo, packetInfo.size(), InetAddress::getLocalHost(), SCHEDULER);
        }
    }
    
};

#endif // FLOOR_H
