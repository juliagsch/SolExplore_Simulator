#include "ev.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream> // For error logging

std::vector<EVRecord> readEVData(const std::string &filename){
    std::vector<EVRecord> records;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return records; // Return an empty vector
    }

    std::string line;

    // Skip the header line
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        EVRecord record;
        std::string token;

        try
        {
            // Parse the day
            std::getline(iss, token, ',');
            record.day = std::stoi(token);

            // Parse the weekday
            std::getline(iss, record.weekday, ',');

            // Parse the departure time
            std::getline(iss, record.departureTime, ',');

            if (record.departureTime == "No trips")
            {
                // Set default values for no-trip days
                record.socOnDeparture = 0.0;
                record.arrivalTime = "";
                record.socOnArrival = 0.0;
                record.distanceKm = 0.0;
                record.travelTimeMin = 0;
            }
            else
            {
                // Parse the SOC on departure
                std::getline(iss, token, ',');
                record.socOnDeparture = std::stod(token);

                // Parse the arrival time
                std::getline(iss, record.arrivalTime, ',');

                // Parse the SOC on arrival
                std::getline(iss, token, ',');
                record.socOnArrival = std::stod(token);

                // Parse the distance
                std::getline(iss, token, ',');
                record.distanceKm = std::stod(token);

                // Parse
                records.push_back(record);
            }

            // Update the nextDepartureTime for each record
            for (size_t i = 0; i < records.size(); ++i){
                if (i + 1 < records.size())
                {
                    records[i].nextDepartureTime = records[i + 1].departureTime;
                }
                else
                {
                    records[i].nextDepartureTime = "No trips";
                }
            }

            return records;



            void updateEVStatus(EVStatus & status, const std::string &currentTime){
                // Update status based on currentTime and EV records
                // ...
                }
