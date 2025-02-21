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

template <typename Type>
class Floor {
private:
    std::string filename;
    Scheduler<Type>& scheduler;
    Scheduler<Type>& elevatorNotifier;

public:
    Floor(const std::string& file, Scheduler<Type>& sched, Scheduler<Type>& notifier)
        : filename(file), scheduler(sched), elevatorNotifier(notifier) {}

    void operator()() {
        std::ifstream file(filename); 
        if (!file) { 
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        std::cout << "[Scheduler] Request input from the Floor Subsystem" << std::endl;
        std::string line;
        while (std::getline(file, line)) { 
            std::istringstream iss(line);
            std::string timeStr, floorButton;
            int floor, floorsToMove;

            if (!(iss >> timeStr >> floor >> floorButton >> floorsToMove)) {
                std::cerr << "Incorrect line format: " << line << std::endl;
                continue;
            }

            struct tm timestamp = {};
            std::istringstream timeStream(timeStr);
            timeStream >> std::get_time(&timestamp, "%H:%M:%S");

            scheduler.put(Type(timestamp, floor, floorButton, floorsToMove));
        }
    }
};

#endif // FLOOR_H
