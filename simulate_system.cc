#include <cmath>
#include <iostream>
#include <vector>
#include <utility>
#include "simulate_system.h"
#include "ev.h"
#include "common.h"

using namespace std;

int loss_events = 0;
double load_deficit = 0;
double load_sum = 0;

double calc_max_charging(double power, double b_prev)
{

	double step = power / 30.0;

	for (double c = power; c >= 0; c -= step)
	{
		double upper_lim = a2_slope * (c / nominal_voltage_c) + 5.0;
		double b = b_prev + c * eta_c * T_u;
		if (b <= upper_lim)
		{
			return c;
		}
	}
	return 0;
}

double calc_max_discharging(double power, double b_prev)
{

	double step = power / 30.0;

	for (double d = power; d >= 0; d -= step)
	{
		double lower_lim = a1_slope * (d / nominal_voltage_d) + 0.0;
		double b = b_prev - d * eta_d * T_u;
		if (b >= lower_lim)
		{
			return d;
		}
	}
	return 0;
}

double calc_max_charging_ev(double power, double ev_b)
{
	double step = power / 30.0;
	double upper_lim = max_soc * ev_battery_capacity;
	double ev_b_m = 0.0;
	for (double c = power; c > 0; c -= step)
	{
		ev_b_m = ev_b + c * eta_c_ev * T_u;
		if (ev_b_m < upper_lim)
		{
			return c;
		}
	}
	return 0;
}

double calc_max_discharging_ev(double power, double ev_b, double min_soc, double ev_battery_size)
{
	double step = power / 30.0;
	double lower_lim = min_soc * ev_battery_capacity;

	for (double d = power; d >= 0; d -= step)
	{
		// Update battery SOC
		ev_b = ev_b - d * eta_d_ev * T_u;

		// Check if the updated SOC is above the lower limit
		if (ev_b >= lower_lim)
		{
			return d;
		}
	}
	return 0;
}

int lastp(const std::vector<EVStatus> &dailyStatuses, double ev_b, int currentHour)
{
	// Find the next departure hour
	int next_dept = -1;
	if (dailyStatuses[currentHour].isAtHome && ev_b < (max_soc * ev_battery_capacity - 0.1))
	{
		next_dept = convertTimeToHour(dailyStatuses[currentHour].nextDepartureTime);
	}
	if (next_dept == -1)
	{
		return -1;
	}

	double diff = ev_battery_capacity * max_soc - ev_b;
	int hours_needed = ceil(diff / charging_rate);
	if (hours_needed == 0)
	{
		return -1; // EV is already charged
	}
	// Compute the latest start time for charging, taking modulo 24 to wrap around midnight
	int latest_t = (next_dept + 24 - hours_needed) % 24;

	return latest_t;
}

std::pair<double, double> unidirectional_static(double b, double ev_b, double c, double d, bool z, double max_c, double max_d, double maxCharging, double is_home, int hour)
{
	if (z == true)
	{
		ev_b = ev_b + maxCharging * eta_c_ev * T_u;
		ev_charged += maxCharging;
		ChargingEvent event;
		event.hour = hour;
		event.chargingAmount = maxCharging;
		chargingEvents.push_back(event);
	}
	if (c > 0)
	{
		b = b + max_c * eta_c * T_u;
		stat_charged += max_c;
	}
	if (d > 0)
	{
		b = b - max_d * eta_d * T_u;
		stat_discharged += max_d;
		if (max_d < d)
		{
			loss_events += 1;
			load_deficit += (d - max_d);
			grid_import += (d - max_d);
		}
	}

	return std::make_pair(ev_b, b);
}

std::pair<double, double> maximise_solar_charging(double b, double ev_b, double c, double d, bool z, double max_c, double max_d, double maxCharging, bool is_home, int hour)
{
	// charge EV whenever there is sun available after laod has been covered

	if (z == true)
	{
		ev_b = ev_b + maxCharging * eta_c_ev * T_u;
		ev_charged += maxCharging;
		ChargingEvent event;
		event.hour = hour;
		event.chargingAmount = maxCharging;
		chargingEvents.push_back(event);
	}
	if (c > 0 && is_home == true && z == false)
	{
		double max_c_ev = calc_max_charging_ev(c, ev_b);
		ev_b = ev_b + max_c_ev * eta_c_ev * T_u;
		ev_charged += max_c_ev;
		if (max_c_ev > 0)
		{
			ChargingEvent event;
			event.hour = hour;
			event.chargingAmount = max_c_ev;
			chargingEvents.push_back(event);
		}
		double res = c - max_c_ev;
		if (res > 0)
		{
			max_c = fmin(calc_max_charging(res, b), alpha_c);
			b = b + max_c * eta_c * T_u;
			stat_charged += max_c;
		}
	}
	else if (c > 0 && is_home == false || c > 0 && z == true)
	{
		b = b + max_c * eta_c * T_u;
		stat_charged += max_c;
	}
	if (d > 0)
	{
		b = b - max_d * eta_d * T_u;
		stat_discharged += max_d;
		double res = d - max_d;
		if (res > 0 && z == false && is_home == true)
		{
			double max_d_ev = fmin(calc_max_discharging_ev(res, ev_b, min_soc, ev_battery_capacity), charging_rate);
			ev_b = ev_b - max_d_ev * eta_c_ev * T_u;
			ev_discharged += max_d_ev;
			res = res - max_d_ev;
		}
		if (res > 0)
		{
			loss_events += 1;
			load_deficit += res;
			grid_import += res;
		}
	}
	return std::make_pair(ev_b, b);
}

double get_maxCharging(double ev_b)
{
	double available_power = charging_rate * T_u;
	return calc_max_charging_ev(available_power, ev_b);
}

double get_ev_b(std::vector<EVStatus> &dailyStatuses, int hour, double last_soc)
{
	double ev_b = 0.0;

	if (!dailyStatuses[hour - 1].isAtHome && dailyStatuses[hour].isAtHome && hour != 0)
	{
		ev_b = dailyStatuses[hour].currentSOC;
	}
	else
	{
		ev_b = last_soc;
	}

	return ev_b;
}

// call it with a specific battery and PV size and want to compute the loss
double sim(vector<double> &load_trace, vector<double> &solar_trace, int start_index, int end_index, double cells, double pv, double b_0, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc, int Ev_start)
{
	// cells is the max battery size
	double b = 0.0;
	// start each simulation with a fully charged battery
	loss_events = 0;
	load_deficit = 0;
	grid_import = 0;
	load_sum = 0;
	total_load = 0;

	int trace_length_solar = solar_trace.size();
	int trace_length_load = load_trace.size();

	double c = 0.0;
	double d = 0.0;
	double c2 = 0.0;
	double d2 = 0.0;
	double max_c = 0.0;
	double max_d = 0.0;
	double max_c2 = 0.0;
	double max_d2 = 0.0;
	double max_c_ev = 0.0;
	double max_d_ev = 0.0;
	int index_t_solar;
	int index_t_load;
	double last_soc = 32.0;
	int day = 0;
	int hour = 0;
	bool charged_last_hour = false;

	int trace_days = min(trace_length_load / 24, trace_length_solar / 24);
	int load_s_Start_day = rand() % trace_days;
	start_index = load_s_Start_day * 24;

	for (int day = 0; day < number_of_chunks; day++)
	{
		int ev_day = day + Ev_start;
		ev_day = ev_day % 365;

		for (int hour = 0; hour < 24; hour++)
		{
			total_hours = total_hours + 1;
			bool z = false;
			int t = day * 24 + start_index + hour;
			index_t_solar = t % trace_length_solar;
			index_t_load = t % trace_length_load;
			load_sum += load_trace[index_t_load];
			bool is_home = allDailyStatuses[ev_day][hour].isAtHome;

			std::pair<double, double> operationResult;
			double ev_b = get_ev_b(allDailyStatuses[ev_day], hour, last_soc);
			double ev_b_before = ev_b;
			double maxCharging;

			if (Operation_policy == "safe_unidirectional")
			{
				if (is_home == true)
				{
					maxCharging = get_maxCharging(ev_b);
				}
				else
				{
					maxCharging = 0.0;
				}
				if (maxCharging > 0)
				{
					z = true;
				}
				// maxCharging = 0.0;
				// z = false;
				// ev_b = 0;
				// is_home = false;
				double hourly_laod = load_trace[index_t_load] + maxCharging;
				total_load += hourly_laod;

				// solar_trace[index_t_solar] = 0; for W+E and E sceanrios
				c = fmax(solar_trace[index_t_solar] * pv - hourly_laod, 0);
				d = fmax(hourly_laod - solar_trace[index_t_solar] * pv, 0);
				// if we use the grid to charge the EV, assume that we first try to maximise charging from solar and covering load from solar

				// we assume we have no battery here
				//!! modify the battery size to be 0
				double after_load = load_trace[index_t_load] - solar_trace[index_t_solar] * pv;
				if (after_load < 0)
				{
					// only need the grid to charge the EV at this hour
					double rest_ev = maxCharging + after_load;
					total_cost = total_cost + rest_ev * 0.07;
				}
				else if (after_load > 0)
				{
					if (hour == 0 || hour == 1 || hour == 2 || hour == 3 || hour == 4)
					{
						// special grid tariff for home load
						total_cost = total_cost + after_load * 0.07;
						total_cost = total_cost + maxCharging * 0.07;
					}
					else
					{
						total_cost = total_cost + after_load * 0.35;
						total_cost = total_cost + maxCharging * 0.07;
					}
				}

				max_c = fmin(calc_max_charging(c, b), alpha_c);
				max_d = fmin(calc_max_discharging(d, b), alpha_d);
				// cout << "BEFORE c: " << c << "- d: " << d << "- max_c: " << max_c << "- max_d: " << max_d << "- ev_b: " << ev_b << "- b : " << b << endl;
				operationResult = unidirectional_static(b, ev_b, c, d, z, max_c, max_d, maxCharging, is_home, hour);
			}
			else if (Operation_policy == "hybrid_bidirectional")
			{
				int chargingHour = lastp(allDailyStatuses[ev_day], ev_b, hour);
				double maxCharging = 0.0;
				if (chargingHour == hour)
				{
					z = true;
					maxCharging = get_maxCharging(ev_b);
				}
				else
				{
					z = false;
				}
				// maxCharging = 0.0;
				// z = false;
				// ev_b = 0;
				// is_home = false;
				double hourly_laod = load_trace[index_t_load] + maxCharging;
				total_load += hourly_laod;
				// solar_trace[index_t_solar] = 0; for W+E and E sceanrios
				c = fmax(solar_trace[index_t_solar] * pv - hourly_laod, 0);
				d = fmax(hourly_laod - solar_trace[index_t_solar] * pv, 0);
				// we assume we have no battery here
				//!! modify the battery size to be 0
				double after_load = load_trace[index_t_load] - solar_trace[index_t_solar] * pv;
				if (after_load < 0)
				{
					// only need the grid to charge the EV at this hour
					double rest_ev = maxCharging + after_load;
					total_cost = total_cost + rest_ev * 0.07;
				}
				else if (after_load > 0)
				{
					if (z == false && is_home == true)
					{
						// try to cover rest load from EV before using the grid
						double max_d_ev = fmin(calc_max_discharging_ev(after_load, ev_b, min_soc, ev_battery_capacity), charging_rate);
						after_load = after_load - max_d_ev;
					}
					if (after_load > 0)
					{
						if (hour == 0 || hour == 1 || hour == 2 || hour == 3 || hour == 4)
						{
							// special grid tariff for home load
							total_cost = total_cost + after_load * 0.07;
							total_cost = total_cost + maxCharging * 0.07;
						}
						else
						{
							total_cost = total_cost + after_load * 0.35;
							total_cost = total_cost + maxCharging * 0.07;
						}
					}
				}

				max_c = fmin(calc_max_charging(c, b), alpha_c);
				max_d = fmin(calc_max_discharging(d, b), alpha_d);
				// cout << "BEFORE c: " << c << "d: " << d << "max_c: " << max_c << "max_d: " << max_d << "ev_b: " << ev_b << "b : " <<b << endl;
				operationResult = maximise_solar_charging(b, ev_b, c, d, z, max_c, max_d, maxCharging, is_home, hour);
			}
			else
			{
				std::cout << "ERROR: Invalid operation policy selected" << std::endl;
			}

			ev_b = operationResult.first;
			b = operationResult.second;
			allDailyStatuses[ev_day][hour].currentSOC = ev_b;
			last_soc = ev_b;
			if (ev_b_before < ev_b)
			{
				charged_last_hour = true;
			}
			else
			{
				charged_last_hour = false;
			}

			if (convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) == hour + 1)
			{
				socValues.push_back(last_soc);
			}
		}
	}

	if (metric == 0)
	{
		// lolp
		return loss_events / ((end_index - start_index) * 1.0);
	}
	else
	{
		// metric == 1, eue
		return load_deficit / (load_sum * 1.0);
	}
}

void simulate(vector<double> &load_trace, vector<double> &solar_trace, int start_index, int end_index, double b_0, std::vector<EVRecord> evRecords, std::vector<std::vector<EVStatus>> allDailyStatuses, double max_soc, double min_soc, int Ev_start)
{
	double loss = 0.0;
	loss = sim(load_trace, solar_trace, start_index, end_index, 0, 4, b_0, evRecords, allDailyStatuses, max_soc, min_soc, Ev_start);
}