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
SimulationResult run_simulations(vector<double> &load, vector<double> &solar, int metric, int chunk_size, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc)
{

	// set random seed to a specific value if you want consistency in results
	srand(10);

	// get number of timeslots in each chunk
	vector<vector<SimulationResult>> results;
	int t_chunk_size = chunk_size * (24 / T_u);

	// get random start times and run simulation on this chunk of data
	// compute all sizing curves
	for (int chunk_num = 0; chunk_num < number_of_chunks; chunk_num += 1)
	{

		// int chunk_start = rand() % max(solar.size()%24,load.size()%24);
		int max_size = std::min(solar.size(), load.size());
		int max_chunks = max_size / 24; // Number of complete 24-hour chunks

		// Generate a random chunk index
		int chunk_index = rand() % max_chunks;

		// Calculate chunk_start
		int chunk_start = chunk_index * 24;
		int Ev_start = rand() % evRecords.size();
		int chunk_end = chunk_start + t_chunk_size;
		vector<SimulationResult> sr = simulate(load, solar, chunk_start, chunk_end, 0, evRecords, allDailyStatuses, max_soc, min_soc, Ev_start);
		// saves the sizing curve for this sample
		results.push_back(sr);
	}

#ifdef DEBUG
	// print all of the curves
	int chunk_index = 1;
	cout << "DEBUG: sizing_curves" << endl;
	for (vector<vector<SimulationResult>>::iterator it = results.begin(); it != results.end(); ++it, ++chunk_index)
	{
		cout << "chunk_" << chunk_index << endl;
		for (vector<SimulationResult>::iterator it2 = it->begin(); it2 != it->end(); ++it2)
		{
			cout << it2->B << "\t" << it2->C << "\t" << it2->cost << endl;
		}
	}
	cout << "DEBUG: sizing_curves_end" << endl;
#endif

	// calculate the chebyshev curves, find the cheapest system along their upper envelope, and return it
	// returns the optimal result
	return calculate_sample_bound(results, epsilon, confidence);
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
	SimulationResult sr = run_simulations(load, solar, metric, days_in_chunk, evRecords, allDailyStatuses, max_soc, min_soc);

	double cost = sr.B / kWh_in_one_cell * B_inv + sr.C * PV_inv;
	cout << "Battery: " << sr.B << " PV: " << sr.C << " Total Cost: " << cost << endl;
	// Runs the simulation with the optimal sizing again to compute the grid cost for the payback-time evaluation.
	int t_chunk_size = days_in_chunk * (24 / T_u);
	double battery_cells = sr.B / kWh_in_one_cell;
	sim(load, solar, 0, t_chunk_size, battery_cells, sr.C, 0, evRecords, allDailyStatuses, max_soc, min_soc, 0, true);

	return 0;
}