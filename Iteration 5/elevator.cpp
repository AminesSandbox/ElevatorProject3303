#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include "elevator.hpp"
#include "elevator_event.hpp"

int main() {

    Elevator<ElevatorEvent> elevator1(69, 1);
    Elevator<ElevatorEvent> elevator2(70, 2);
    Elevator<ElevatorEvent> elevator3(471, 3);
    Elevator<ElevatorEvent> elevator4(472, 4);

    std::thread elevator1Thread(std::ref(elevator1));
    std::thread elevator2Thread(std::ref(elevator2));
    std::thread elevator3Thread(std::ref(elevator3));
    std::thread elevator4Thread(std::ref(elevator4));
    
    elevator1Thread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    elevator2Thread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    elevator3Thread.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    elevator4Thread.join();
}
