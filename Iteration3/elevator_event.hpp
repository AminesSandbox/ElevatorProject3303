/*
 * SYSC 3303 B1-Group 1 
 * Created by Hasan Suriya
 * Reviewed by Abdullah Arid
 * 
 * This header file defines a structure to represent an elevator event. The
 * ElevatorEvent structure contains fields for the timestamp, floor number,
 * direction, and number of floors to move. It also includes a method to 
 * display the event details as a string.
 * 
*/

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

    ElevatorEvent(struct tm t, int f, std::string fb, int cb)
        : timestamp(t), floor(f), floorButton(std::move(fb)), floorsToMove(cb) {}

    std::string display() const {
        std::ostringstream oss;
        oss << "[Floor Subsystem] Request sent: Floor " << floor
            << " going " << (floorButton.empty() ? "None" : floorButton)
            << " by " << floorsToMove << " floors";
        return oss.str();
    }
};

#endif // ELEVATOR_EVENT_H
