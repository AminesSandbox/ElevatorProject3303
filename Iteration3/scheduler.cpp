#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include "scheduler.hpp"
#include "elevator_event.hpp"

#define SCHEDULER 23
#define ELEVATOR_1 69
#define ELEVATOR_2 70

void floorReader(Scheduler<ElevatorEvent>* scheduler) {

    for (int i = 0; i < 5; i++) {
        std::vector<uint8_t> data = scheduler->receiveClient();
        ElevatorEvent packetInfo = scheduler->processData(data);
        scheduler->put(packetInfo);
    }
}

void alertElevator(Scheduler<ElevatorEvent>* scheduler) {

    for (int i = 0; i < 5; i++) {
        ElevatorEvent event = scheduler->get();
        std::vector<uint8_t> data = scheduler->createData(event);
        int result = 0;
        if (i % 2 == 0) {
            result = scheduler->sendPacket(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_1);
        }
        else {
            result = scheduler->sendPacket(data, data.size(), InetAddress::getLocalHost(), ELEVATOR_2);
        }
    }

}

int main() {

    std::cout << "[Scheduler] Request input from the Floor Subsystem" << std::endl;
    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    std::thread floorThread(floorReader, &scheduler);
    //Move later
    floorThread.join();

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
    elevatorThread.join();
}

