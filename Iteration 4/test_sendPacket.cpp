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

TEST_CASE ("Sending Packet from Floor to Scheduler") {
    Scheduler<ElevatorEvent> scheduler(23);
    Scheduler<ElevatorEvent> floorNotifier(24);
    Floor<ElevatorEvent> floorReader("elevator.txt");

    //Sending floor packet to scheduler
    std::vector<uint8_t> packetToSend = floorReader.createData("12:00:00:000", "Up", 1, 5);
    floorNotifier.sendPacket(packetToSend, packetToSend.size(), InetAddress::getLocalHost(), SCHEDULER);

    // Simulate receiving and processing the packet in Scheduler
    std::vector<uint8_t> receivedPacket = scheduler.receiveClient();

    CHECK(packetToSend == receivedPacket);
}

TEST_CASE ("Sending Packet from Scheduler to Elevator") {
    Scheduler<ElevatorEvent> scheduler(23);
    Elevator<ElevatorEvent> elevator(69, 1);

    //Sending packet to elevator
    std::vector<uint8_t> packetToSend = scheduler.createData(ElevatorEvent(std::tm(), 1, "Up", 5));
    scheduler.sendPacket(packetToSend, packetToSend.size(), InetAddress::getLocalHost(), ELEVATOR_1);

    // Simulate receiving and processing the packet in Elevator
    std::vector<uint8_t> receivedPacket = elevator.receivePacket();

    CHECK(packetToSend == receivedPacket);
}

TEST_CASE ("Sending Packet from Elevator to Scheduler") {
    Scheduler<ElevatorEvent> floorNotifier(24);
    Elevator<ElevatorEvent> elevator(69, 1);

    std::vector<uint8_t> success_msg(14,1);
    elevator.sendPacket(success_msg, success_msg.size(), InetAddress::getLocalHost(), FLOORNOTIFIER);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::vector<uint8_t> receivedPacket = floorNotifier.receiveClient();

    CHECK(success_msg == receivedPacket);
}
