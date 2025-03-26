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

// chunk_size: length of time (in days)
void run_simulations(vector<double> &load, vector<double> &solar, int metric, int chunk_size, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc)
{

	// set random seed to a specific value if you want consistency in results
	srand(10);

	// get number of timeslots in each chunk
	// zb 100 days a 24h if we have hourly data in the input files
	int t_chunk_size = chunk_size * (24 / T_u);
	cout << "t_chunk_size = " << t_chunk_size << endl;

	vector<vector<SimulationResult>> results;
	int Ev_start = rand() % evRecords.size();
	// to start on a Monday
	Ev_start = 0;
	// set battery to 0 if no stationary storage
	battery_result = 0;
	pv_result = 4;

	sim(load, solar, 0, t_chunk_size, 0, 0, 0, evRecords, allDailyStatuses, max_soc, min_soc, Ev_start);
}

int main(int argc, char **argv)
{
	int input_process_status = process_input(argv, true);

	// Handle input processing error if needed
	if (input_process_status != 0)
	{
		std::cerr << "Error processing input" << std::endl;
		return 1; // Or handle the error as appropriate
	}

	// Read EV data
	std::vector<EVRecord> evRecords = readEVData(path_to_ev_data);

	// Check if EV data was read successfully
	if (evRecords.empty())
	{
		std::cerr << "Error reading EV data or no records found" << std::endl;
		return 1; // Or handle the error as appropriate
	}

	// Initialize EVStatus
	EVStatus evStatus;
	// Generate all daily statuses
	std::vector<std::vector<EVStatus>> allDailyStatuses = generateAllDailyStatuses(evRecords);

	run_simulations(load, solar, metric, days_in_chunk, evRecords, allDailyStatuses, max_soc, min_soc);
	cout << fixed << "Grid import: " << grid_import << endl;
	cout << "Total Cost: " << total_cost << endl;
	cout << "Total Hours: " << total_hours << endl;
	cout << "Total load: " << total_load << endl;
	cout << "EV Power Usage: " << ev_power_used << endl;
	cout << "Total Household Load: " << load_sum << endl;
	cout << "Power Lost: " << power_lost << endl;
	cout << "EV Power Charged: " << max_charging_total << endl;
	cout << "EV Battery Diff: " << ev_battery_diff << endl;
	return 0;
}
