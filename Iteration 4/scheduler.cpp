#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <functional>
#include "scheduler.hpp"
#include "elevator.hpp"
#include "elevator_event.hpp"

#define SCHEDULER 23
//#define ELEVATOR_1 69
//#define ELEVATOR_2 70

#ifndef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
int main() {
    std::cout << "[Scheduler] Request input from the Floor Subsystem" << std::endl;

    Elevator<ElevatorEvent> elevator1(71, 1);
    Elevator<ElevatorEvent> elevator2(72, 2);

    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);

    std::thread floorThread(floorReader, &scheduler);
    std::thread elevatorThread(alertElevator, &scheduler);

    while (true) {
        std::vector<uint8_t> data = floorNotifier.receiveClient();
        if (static_cast<int>(data[0]) == 1 && static_cast<int>(data[1]) == 1) {
            std::cout << "[Scheduler] Request completed\n";
        }
        else {
            ElevatorEvent packetInfo = floorNotifier.processData(data);
            floorNotifier.put(packetInfo);
        }
    }

    floorThread.join();
    elevatorThread.join();
}

#endif