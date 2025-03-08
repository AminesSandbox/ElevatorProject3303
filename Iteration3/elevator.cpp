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
    std::thread elevator1Thread(std::ref(elevator1));
    std::thread elevator2Thread(std::ref(elevator2));
    elevator1Thread.join();
    elevator2Thread.join();
}
