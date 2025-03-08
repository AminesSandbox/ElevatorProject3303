#include <chrono>
#include <thread>
#include <cstdlib>
#include <unistd.h>
#include "floor.hpp"

int main() {

    Floor<ElevatorEvent> floorReader("elevator.txt");
    std::thread floorThread(std::ref(floorReader));
    floorThread.join();
}
