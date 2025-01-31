#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <vector>
#include <stdexcept>

struct ElevatorEvent {
    std::chrono::milliseconds time; // Time stored in milliseconds
    int floor;                      // Floor number
    std::string floorButton;        // "up", "down", or empty string ""
    int carButton;                  // Holds an integer value for the car button

    // Constructor with validation
    ElevatorEvent(std::chrono::milliseconds t, int f, std::string fb, int cb) 
        : time(t), floor(f), carButton(cb) {
        
        // Validate floorButton input
        if (fb != "up" && fb != "down" && !fb.empty()) {
            throw std::invalid_argument("Invalid floor button value. Must be 'up', 'down', or empty.");
        }
        floorButton = std::move(fb);
    }

    // Function to format time as hh:mm:ss.mmm
    std::string getFormattedTime() const {
        using namespace std::chrono;

        // Extract total milliseconds as a raw value
        auto total_ms = time.count();

        // Compute hours, minutes, seconds, and milliseconds
        int hours = total_ms / (1000 * 60 * 60);
        total_ms %= (1000 * 60 * 60);
        int minutes = total_ms / (1000 * 60);
        total_ms %= (1000 * 60);
        int seconds = total_ms / 1000;
        int milliseconds = total_ms % 1000;

        // Format output
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << seconds << "."
            << std::setw(3) << std::setfill('0') << milliseconds;

        return oss.str();
    }

    // Display function for debugging
    void display() const {
        std::cout << "Time: " << getFormattedTime() 
                  << ", Floor: " << floor 
                  << ", Floor Button: " << (floorButton.empty() ? "None" : floorButton) 
                  << ", Car Button: " << carButton 
                  << std::endl;
    }
};

// Class to manage a list of ElevatorEvents
class ElevatorEventLog {
private:
    std::vector<ElevatorEvent> events;

public:
    // Add an event with validation
    void addEvent(std::chrono::milliseconds time, int floor, const std::string& floorButton, int carButton) {
        try {
            events.emplace_back(time, floor, floorButton, carButton);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error adding event: " << e.what() << std::endl;
        }
    }

    // Display all stored events
    void displayAllEvents() const {
        if (events.empty()) {
            std::cout << "No events logged." << std::endl;
            return;
        }

        std::cout << "Elevator Event Log:\n";
        for (const auto& event : events) {
            event.display();
        }
    }
};

int main() {
    ElevatorEventLog log;

    // Example events
    log.addEvent(std::chrono::hours(3) + std::chrono::minutes(15) + std::chrono::seconds(42) + std::chrono::milliseconds(250),
                 5, "up", 2);

    log.addEvent(std::chrono::hours(4) + std::chrono::minutes(5) + std::chrono::seconds(10) + std::chrono::milliseconds(500),
                 7, "down", 3);

    log.addEvent(std::chrono::hours(6) + std::chrono::minutes(30) + std::chrono::seconds(20) + std::chrono::milliseconds(750),
                 2, "", 1);  // No floor button pressed

    log.addEvent(std::chrono::hours(7) + std::chrono::minutes(45) + std::chrono::seconds(5) + std::chrono::milliseconds(300),
                 10, "left", 4); // Invalid input, should trigger an error

    // Display all events
    log.displayAllEvents();

    return 0;
}

