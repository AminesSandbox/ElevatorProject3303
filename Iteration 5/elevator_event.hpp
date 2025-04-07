#ifndef ELEVATOR_EVENT_H
#define ELEVATOR_EVENT_H

#include <string>
#include <sstream>
#include <iomanip>

struct ElevatorEvent {
    struct tm timestamp;
    int floor;
    std::string floorButton;
    int floorsToMove;
    int passengers;
    std::string fault;

    static ElevatorEvent parseFromPacket(const std::vector<uint8_t>& data) {
        if (data.size() < 17 || data[0] != 0 || data[1] != 1) {
            throw std::runtime_error("Invalid packet");
        }
    
        int hour = data[2] * 10 + data[3];
        int min = data[4] * 10 + data[5];
        int sec = data[6] * 10 + data[7];
        struct tm timestamp = {hour = hour, min = min, sec = sec};
    
        int floor = data[9] * 10 + data[10];
        std::string direction = (data[11] == 1) ? "Up" : "Down";
        int floorsToMove = data[12] * 10 + data[13];
        int passengers = data[14] * 10 + data[15];
        std::string fault;
        switch (data[16]) {
            case 1: fault = "Minor"; break;
            case 2: fault = "Major"; break;
            default: fault = "None"; break;
        }
    
        return ElevatorEvent(timestamp, floor, direction, floorsToMove, passengers, fault);
    }
        

    ElevatorEvent(struct tm t, int f, std::string fb, int cb, int p, std::string fa) : timestamp(t), floor(f), floorButton(std::move(fb)), floorsToMove(cb), passengers (p), fault(fa) {}

   
    std::string display() const {
        std::ostringstream oss;
        oss << "\n [Floor Subsystem] Request sent:\n"
            << "  Floor: " << floor
            << "\n  Direction: " << (floorButton.empty() ? "None" : floorButton)
            << "\n  Floors to move: " << floorsToMove
            << "\n  Passengers: " << passengers
            << "\n  Fault: " << fault;

        return oss.str();
    }
};

#endif 
