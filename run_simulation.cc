#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include "simulate_system.h"
#include "common.h"
#include "cheby.h"
#include "ev.h"

using namespace std;

// run_simulations
// load_filename: filename, each line in file contains electricity consumption value
// solar_filename: filename, each line in file contains solar generation value
// id: request id
// metric: 0 for LOLP, 1 for unmet load
// epsilon: number in range [0,1] representing LOLP or unmet load fraction.
// chunk_size: length of time (in days)
void run_simulations(vector<double> &load, vector<double> &solar, int metric, int chunk_size, int number_of_chunks, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc) {

	// set random seed to a specific value if you want consistency in results
	srand(10);

	// get number of timeslots in each chunk
	//zb 100 days a 24h if we have hourly data in the input files 
	int t_chunk_size = chunk_size*(24/T_u);
	cout << "t_chunk_size = " << t_chunk_size << endl;

	vector <vector<SimulationResult> > results;

	// get random start times and run simulation on this chunk of data
	// compute all sizing curves
	//for (int chunk_num = 0; chunk_num < number_of_chunks; chunk_num += 1) {

		//int chunk_start = rand() % max(solar.size()%24,load.size()%24);
		//int max_size = std::min(solar.size(), load.size());
		//int max_chunks = max_size / 24; // Number of complete 24-hour chunks

		// Generate a random chunk index
		//int chunk_index = rand() % max_chunks;

		// Calculate chunk_start
		//int chunk_start = chunk_index * 24;
		//TODO: modify this if we know the first day of the chunk is for e.g. a monday, then ev_Start should also be a monday
		int Ev_start = rand() % evRecords.size();
		// to start on a Monday
		Ev_start = 0;
		//int chunk_end = chunk_start + t_chunk_size;
		battery_result = 0;
		pv_result = 4;

		sim(load, solar, 0, t_chunk_size, battery_result, pv_result, 0, evRecords, allDailyStatuses, max_soc, min_soc, Ev_start);

		//saves the sizing curve for this sample 


	}



int main(int argc, char ** argv) {
	int input_process_status = process_input(argv, true);

	// Handle input processing error if needed
	if (input_process_status != 0){
		std::cerr << "Error processing input" << std::endl;
		return 1; // Or handle the error as appropriate
	}

	// Read EV data
	std::vector<EVRecord> evRecords = readEVData(path_to_ev_data);

	// Check if EV data was read successfully
	if (evRecords.empty()){
		std::cerr << "Error reading EV data or no records found" << std::endl;
		return 1; // Or handle the error as appropriate
	}
	
		// Initialize EVStatus
		EVStatus evStatus;
	//printEVRecords(evRecords);

	// Generate all daily statuses
	std::vector<std::vector<EVStatus>> allDailyStatuses = generateAllDailyStatuses(evRecords);
	//printAllEVStatuses(allDailyStatuses, evRecords);
	//printAllEVStatusesToCSV(allDailyStatuses, evRecords, "output.csv"); // Replace "output.csv" with your desired file name

	//write a cout command that prints out the evcharging polocy and operation policy
	//cout << "EV charging policy: " << EV_charging << endl;
	//cout << "EV operation policy: " << Operation_policy << endl;

	run_simulations(load, solar, metric, days_in_chunk, number_of_chunks, evRecords, allDailyStatuses, max_soc, min_soc);
							 // Replace with actual load number, possibly parsed from input

	// Construct the output filename
	/*
	std::stringstream filename;
	//cout << "wfh_type = " << wfh_type << endl;
	filename << "soc/soc_" << Operation_policy << "_" << loadNumber << "_" << wfh_type << ".txt";

	// Open an output file stream with the constructed filename
	std::ofstream outFile(filename.str());
	if (!outFile)
	{
		std::cerr << "Error: Unable to open file for writing." << std::endl;
		return 1;
	}

	// Write SOC values to the file
	for (const auto &value : socValues)
	{
		outFile << value << std::endl;
	}

	// Close the file stream
	outFile.close();

	std::stringstream filename2;
	filename2 << "charging/c_" << Operation_policy << "_" << loadNumber << "_" << wfh_type << ".txt";

	// Open an output file stream with the constructed filename
	std::ofstream outFile2(filename2.str());
	if (!outFile)
	{
		std::cerr << "Error: Unable to open file for writing." << std::endl;
		return 1;
	}

	// Write charging data to the file
	for (const auto &event : chargingEvents)
	{
		outFile2 << event.hour << "\t" << event.chargingAmount << std::endl;
	}

	// Close the file stream
	outFile2.close();
	*/

	// TODO: Add output file for kwH values
	// output  ev_charged, ev_discharged, stat_charged, stat_discharged
	//cout << ev_charged << "\t" << ev_discharged << "\t" << stat_charged << "\t" << stat_discharged << endl;
	cout << "Grid import: " << grid_import << endl;
	cout << "Total load: " << total_load << endl;
// evval10_interum_use.py will store all outputs in a csv file
	return 0;


}
