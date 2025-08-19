// params.h
#ifndef PARAMS_H
#define PARAMS_H

#include "params_common.h"

extern double PV_inv; // cost per unit (kW) of PV
extern vector<double> solar;

// define the upper and lower values to test for pv,
// as well as the step size of the search
extern double pv_min;
extern double pv_max;
extern double pv_step; // search in steps of x kW

extern double grid_import;
extern double total_load;
extern double total_cost;
extern double total_hours;
extern double load_sum;           // Total load used
extern double ev_power_used;      // Total power used by ev driving (discharging to power house not included)
extern double power_lost;         // Electricity lost due to charging and discharging efficiencies
extern double max_charging_total; // Total electricity used to charge the EV
extern double ev_battery_diff;    // EV battery difference between beginning and start of the simulation

extern double min_soc;
extern double max_soc;
extern double ev_battery_capacity;
extern double charging_rate;
extern double discharging_rate;
extern double min_battery_charge;

extern std::string Operation_policy;
extern std::string path_to_ev_data;
extern int loadNumber;
extern std::string wfh_type;

struct SimulationResult
{
    double B;
    double C;
    double cost;

    SimulationResult(double B_val, double C_val, double cost_val) : B(B_val), C(C_val), cost(cost_val) {}
};

struct OperationResult
{
    double ev_b;
    double b;
    double ev_charged;
    OperationResult(double ev_b_val, double b_val, double ev_charged_val) : ev_b(ev_b_val), b(b_val), ev_charged(ev_charged_val) {}
};

extern std::vector<double> socValues;

int process_input(int argc, char **argv, bool process_metric_input);

#endif
