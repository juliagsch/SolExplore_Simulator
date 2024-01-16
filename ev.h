// ev.h
#ifndef EV_H
#define EV_H

#include <string>
#include <vector>


struct EVRecord
{
    int day;
    std::string weekday;
    std::string departureTime;
    double socOnDeparture;
    std::string arrivalTime;
    double socOnArrival;
    double distanceKm;
    int travelTimeMin;
    std::string nextDepartureTime; 
};

extern std::vector<EVRecord> evRecords;

struct EVStatus
{
    bool isAtHome;
    std::string nextDepartureTime;
    double currentSOC;
};

// Function declarations
std::vector<EVRecord> readEVData(const std::string &filename);
void updateEVStatus(EVStatus &status, const std::string &currentTime);

#endif // EV_H
