#include <iostream>
#include <map>
#include <thread>
#include <chrono>
#include <unistd.h>
#include "elevator.hpp"


std::string stateToStr(int s) {
    switch (static_cast<ElevatorState>(s)) {
        case ElevatorState::Idle: return "IDLE";
        case ElevatorState::MovingUp: return "MOVING UP";
        case ElevatorState::MovingDown: return "MOVING DOWN";
        case ElevatorState::DoorOpening: return "DOOR OPENING";
        case ElevatorState::DoorOpen: return "DOOR OPEN";
        case ElevatorState::DoorClosing: return "DOOR CLOSING";
        case ElevatorState::MinorFault: return "MINOR FAULT";
        case ElevatorState::MajorFault: return "SHUTDOWN";
        default: return "UNKNOWN";
    }
}

std::string dirToStr(int d) {
    if (d == 1) return "UP";
    if (d == 2) return "DOWN";
    return "IDLE";
}

int main() {
    DatagramSocket displaySocket(DISPLAY_PORT);
    std::map<int, std::tuple<int, std::string, std::string>> statusMap;

    while (true) {
        std::vector<uint8_t> buf(4);
        DatagramPacket pkt(buf, buf.size());
        displaySocket.receive(pkt);

        int id = buf[0];
        int floor = buf[1];
        std::string dir = dirToStr(buf[2]);
        std::string state = stateToStr(buf[3]);

        statusMap[id] = std::make_tuple(floor, dir, state);

        std::cout << "\033c"; // clear console
        std::cout << "--- ELEVATOR STATUS CONSOLE ---" << std::endl;
        for (const auto& [eid, tup] : statusMap) {
            auto [f, d, s] = tup;
            std::cout << "[Elevator " << eid << "] Floor: " << f 
                      << "  | Dir: " << d 
                      << " | Status: " << s << std::endl;
        }
        std::cout << "---------------------------------" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
