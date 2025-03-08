#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "scheduler.hpp"
#include "elevator_event.hpp"
#include "elevator.hpp"
#include "floor.hpp"

#include <thread>
#include <iostream>
#include <chrono>
#include <sstream>

TEST_CASE ("Packet Being Sent") {
    Scheduler<ElevatorEvent> scheduler(23);
    Floor<ElevatorEvent> floor("requests.txt");
    Elevator<ElevatorEvent> elevator(69, 1);

    std::thread schedulerThread([&scheduler]() { floorReader(&scheduler); });
    std::thread elevatorThread([&elevator]() { elevator(); });

    // Simulate sending a packet from Floor to Scheduler
    std::vector<uint8_t> packet = floor.createData("12:00:00:000", "Up", 1, 5);
    floor.sendPacket(packet, packet.size(), InetAddress::getLocalHost(), SCHEDULER);

    // Simulate receiving and processing the packet in Scheduler
    std::vector<uint8_t> receivedPacket = scheduler.receiveClient();
    ElevatorEvent event = scheduler.processData(receivedPacket);
    scheduler.put(event);

    // Simulate Elevator processing the request
    std::this_thread::sleep_for(std::chrono::seconds(1));
    CHECK(elevator.getCurrentFloor() == 1);

    schedulerThread.join();
    elevatorThread.join();
}
