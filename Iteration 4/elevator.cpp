#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include "elevator.hpp"
#include "elevator_event.hpp"

/*
Will insert a majorfault half a second after elevator reaches specified state
*/
void insertMajorFault(Elevator<ElevatorEvent>* elevator, ElevatorState state) {
    while (true) {
        if(elevator->getState() == state) {

            std::cout << "Elevator state before fault: " << static_cast<int>(elevator->getState()) << std::endl;
            
            std::cout << "Fault inserted"<< std::endl;
            elevator->setState(ElevatorState::MajorFault);
            
            std::cout << "Elevator state after fault: " << static_cast<int>(elevator->getState()) << std::endl;

            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

/*
Will insert a minorfault half a second after elevator reaches specified state
*/
void insertMinorFault(Elevator<ElevatorEvent>* elevator, ElevatorState state) {
    while (true) {
        if(elevator->getState() == state) {

            std::cout << "Elevator state before fault: " << static_cast<int>(elevator->getState()) << std::endl;
            
            std::cout << "Fault inserted"<< std::endl;
            elevator->setState(ElevatorState::MinorFault);
            
            std::cout << "Elevator state after fault: " << static_cast<int>(elevator->getState()) << std::endl;

            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


int main() {

    Elevator<ElevatorEvent> elevator1(69, 1);

    std::thread faultInjector(insertMinorFault, &elevator1, ElevatorState::MovingUp);


    Elevator<ElevatorEvent> elevator2(70, 2);
    std::thread elevator1Thread(std::ref(elevator1));

    std::thread elevator2Thread(std::ref(elevator2));
    elevator1Thread.join();
    elevator2Thread.join();
}
