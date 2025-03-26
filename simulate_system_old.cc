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

void update_parameters(double n, double cells)
{

    num_cells = n;

    a1_intercept = 0.0 * num_cells;

    a2_intercept = cells;

    alpha_d = a2_intercept * 1.0;
    alpha_c = a2_intercept * 1.0;
    return;
}

double calc_max_charging(double power, double b_prev)
{

    double step = power / 30.0;

    for (double c = power; c >= 0; c -= step)
    {
        double upper_lim = a2_slope * (c / nominal_voltage_c) + a2_intercept;
        // cout << "upper_lim: " << upper_lim << "c: " << c << endl;
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
        double lower_lim = a1_slope * (d / nominal_voltage_d) + a1_intercept;
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

int naive(const std::vector<EVStatus> &dailyStatuses, double ev_b, int currentHour)
{
    // Check if the EV is at home and can be charged
    // TODO: improve this 0.2
    if (dailyStatuses[currentHour].isAtHome && ev_b < (max_soc * ev_battery_capacity - 0.2))
    {
        return currentHour; // Start charging
    }
    return -1; // Do not charge if the EV is not at home
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

int mincost(const std::vector<EVStatus> &dailyStatuses, double ev_b, int currentHour, int t_ch, bool charged_last_hour)
{
    // Check if the EV is at home at the lowest cost hour
    if (charged_last_hour && (ev_b < (max_soc * ev_battery_capacity - 0.1)) && dailyStatuses[currentHour].isAtHome)
    {
        return currentHour; // Start charging
    }
    if (dailyStatuses[currentHour].isAtHome && dailyStatuses[currentHour].nextDepartureTime != "No trips")
    {
        return t_ch; // Start charging
    }

    return -1; // Do not charge if the EV is not at home at t_ch
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

std::pair<double, double> no_ev(double b, double ev_b, double c, double d, bool z, double max_c, double max_d, double maxCharging, double is_home, int hour)
{

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

std::pair<double, double> unidirectional_dynamic(double b, double ev_b, double d2, double max_d2, double c2, double max_c2, bool is_home, int hour)
{

    if (c2 > 0 && is_home == true)
    {
        double max_c_ev = calc_max_charging_ev(c2, ev_b);
        ev_b = ev_b + max_c_ev * eta_c_ev * T_u;
        ev_charged += max_c_ev;
        if (max_c_ev > 0)
        {
            ChargingEvent event;
            event.hour = hour;
            event.chargingAmount = max_c_ev;
            chargingEvents.push_back(event);
        }
        double res = c2 - max_c_ev;
        if (res > 0)
        {
            max_c2 = fmin(calc_max_charging(res, b), alpha_c);
            b = b + max_c2 * eta_c * T_u;
            stat_charged += max_c2;
        }
    }
    else if (c2 > 0 && is_home == false)
    {
        b = b + max_c2 * eta_c * T_u;
        stat_charged += max_c2;
    }
    if (d2 > 0)
    {
        b = b - max_d2 * eta_d * T_u;
        stat_discharged += max_d2;
        if (max_d2 < d2)
        {
            loss_events += 1;
            load_deficit += (d2 - max_d2);
            grid_import += (d2 - max_d2);
        }
    }

    return std::make_pair(ev_b, b);
}

std::pair<double, double> unidirectional_hybrid(double b, double ev_b, double c, double d, bool z, double max_c, double max_d, double maxCharging, double is_home, int hour)
{
    if (z == true)
    {
        ev_b = ev_b + maxCharging * eta_c_ev * T_u;
        ChargingEvent event;
        event.hour = hour;
        event.chargingAmount = maxCharging;
        chargingEvents.push_back(event);
        ev_charged += maxCharging;
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

std::pair<double, double> maximise_solar_charging_safe(double b, double ev_b, double c, double d, double max_c, double max_d, bool is_home, bool dont_discharge, bool z, double maxCharging, int hour)
{
    // charge EV whenever there is sun available after laod has been covered
    double res_c;
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
        res_c = c - max_c_ev;
        if (res_c > 0)
        {
            max_c = fmin(calc_max_charging(res_c, b), alpha_c);
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
        if (res > 0 && z == false && is_home == true && dont_discharge == false)
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

std::pair<double, double> bidirectional_optimal(double b, double ev_b, double c2, double d2, bool z, double max_c2, double max_d2, bool is_home, int hour)
{
    // charge EV whenever there is sun available after laod has been covered
    double max_d_ev;

    if (c2 > 0 && is_home == true)
    {
        double max_c_ev = calc_max_charging_ev(c2, ev_b);
        ev_b = ev_b + max_c_ev * eta_c_ev * T_u;
        ev_charged += max_c_ev;
        if (max_c_ev > 0)
        {
            ChargingEvent event;
            event.hour = hour;
            event.chargingAmount = max_c_ev;
            chargingEvents.push_back(event);
            total_load += max_c_ev;
        }
        double res = c2 - max_c_ev;
        if (res > 0)
        {
            max_c2 = fmin(calc_max_charging(res, b), alpha_c);
            b = b + max_c2 * eta_c * T_u;
            stat_charged += max_c2;
        }
    }
    else if (c2 > 0 && is_home == false)
    {
        b = b + max_c2 * eta_c * T_u;
        stat_charged += max_c2;
    }
    if (d2 > 0)
    {
        b = b - max_d2 * eta_d * T_u;
        stat_discharged += max_d2;
        double res = d2 - max_d2;
        if (res > 0 && is_home == true)
        {
            max_d_ev = fmin(calc_max_discharging_ev(res, ev_b, min_soc, ev_battery_capacity), charging_rate);
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

    double num_cells2 = cells / kWh_in_one_cell;
    update_parameters(num_cells2, cells);

    // cells is the max battery size

    double b = cells;
    // set storage to 4kwh for our experiment:
    // b = 5.0;
    // start each simulation with a fully charged battery
    loss_events = 0;
    load_deficit = 0;
    // VERIFY
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
    int EV_index = Ev_start;

    // loop through each hour in the sample
    // t is the day
    // h is the hour
    // index t = day * 24 + hour
    int trace_days = min(trace_length_load / 24, trace_length_solar / 24);
    int load_s_Start_day = rand() % trace_days;
    start_index = load_s_Start_day * 24;
    for (int day = 0; day < days_in_chunk; day++)
    {

        int ev_day = day + Ev_start;
        ev_day = ev_day % 365;
        // cout << "Day: " << day << endl;
        for (int hour = 0; hour < 24; hour++)
        {
            bool z = false;

            int t = day * 24 + start_index + hour;
            index_t_solar = t % trace_length_solar;
            index_t_load = t % trace_length_load;

            load_sum += load_trace[index_t_load];

            bool is_home = allDailyStatuses[ev_day][hour].isAtHome;

            c2 = fmax(solar_trace[index_t_solar] * pv - load_trace[index_t_load], 0);
            d2 = fmax(load_trace[index_t_load] - solar_trace[index_t_solar] * pv, 0);
            // cout << "c2 : " << c2 << "d2: " << d2 <<  "b: " << b << endl;
            max_c2 = fmin(calc_max_charging(c2, b), alpha_c);
            // cout << "operation policy3" << Operation_policy << "load_deficit: " << load_deficit << "load_sum" << load_sum << endl;

            max_d2 = fmin(calc_max_discharging(d2, b), alpha_d);
            // cout << "operation policy4" << Operation_policy << "load_deficit: " << load_deficit << "load_sum" << load_sum << endl;

            std::pair<double, double> operationResult;
            double ev_b = get_ev_b(allDailyStatuses[ev_day], hour, last_soc);
            double ev_b_before = ev_b;
            double maxCharging;
            // cout << "BEFORE day: " << day << "hour: " << hour << "ev_b: " << ev_b << "b: " << b << "isHome: " << is_home << endl;
            // cout << "BEFORE c2 without ev load: " << c2 << "d2: " << d2 << "max_c2: " << max_c2 << "max_d2: " << max_d2 << endl;

            if (Operation_policy == "optimal_unidirectional")
            {
                // only charge EV with excess solar energy, but not guaranteed that EV will be fully charged
                // cout << "EV DAY: " << allDailyStatuses[ev_day][hour].dayNumber << "hour: " << hour << "nextDept: " << convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) << endl;
                // cout << "BEFORE c2: " << c2 << "d2: " << d2 << "max_c2: " << max_c2 << "max_d2: " << max_d2 << "ev_b: " << ev_b << "b : " << b << endl;

                operationResult = unidirectional_dynamic(b, ev_b, d2, max_d2, c2, max_c2, is_home, hour);
                // cout << "AFTER ev_b: " << operationResult.first << "b: " << operationResult.second << endl;
            }
            else if (Operation_policy == "no_ev")
            {
                // cout << "EV DAY: " << allDailyStatuses[ev_day][hour].dayNumber << "hour: " << hour << "nextDept: " << convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) << endl;

                maxCharging = 0.0;

                double hourly_laod = load_trace[index_t_load] + maxCharging;
                total_load += hourly_laod;
                c = fmax(solar_trace[index_t_solar] * pv - hourly_laod, 0);
                // c = 0; for W+E and E sceanrios

                d = fmax(hourly_laod - solar_trace[index_t_solar] * pv, 0);
                max_c = fmin(calc_max_charging(c, b), alpha_c);
                max_d = fmin(calc_max_discharging(d, b), alpha_d);
                // cout << "BEFORE c: " << c << "d: " << d << "max_c: " << max_c << "max_d: " << max_d << "ev_b: " << ev_b << "b : " << b << endl;

                operationResult = no_ev(b, ev_b, c, d, z, max_c, max_d, maxCharging, is_home, hour);
                // cout << "AFTER ev_b: " << operationResult.first << "b: " << operationResult.second << endl;
            }
            else if (Operation_policy == "safe_unidirectional")
            {
                // cout << "EV DAY: " << allDailyStatuses[ev_day][hour].dayNumber << "hour: " << hour << "nextDept: " << convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) << endl;

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
                // solar_trace[index_t_solar] = 0;
                c = fmax(solar_trace[index_t_solar] * pv - hourly_laod, 0);
                // c = 0; for W+E and E sceanrios

                d = fmax(hourly_laod - solar_trace[index_t_solar] * pv, 0);
                max_c = fmin(calc_max_charging(c, b), alpha_c);
                max_d = fmin(calc_max_discharging(d, b), alpha_d);
                cout << "BEFORE c: " << c << "d: " << d << "max_c: " << max_c << "max_d: " << max_d << "ev_b: " << ev_b << "b : " << b << endl;

                operationResult = unidirectional_static(b, ev_b, c, d, z, max_c, max_d, maxCharging, is_home, hour);
                // cout << "AFTER ev_b: " << operationResult.first << "b: " << operationResult.second << endl;
            }
            else if (Operation_policy == "hybrid_unidirectional")
            {

                int chargingHour = lastp(allDailyStatuses[ev_day], ev_b, hour);
                double maxCharging = 0.0;
                // cout << "EV DAY: " << allDailyStatuses[ev_day][hour].dayNumber << "hour: " << hour << "nextDept: " << convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) << endl;
                // cout << "chargingHour: " << chargingHour << "currentHour: " << hour << endl;
                if (chargingHour == hour)
                {
                    z = true;
                    maxCharging = get_maxCharging(ev_b);
                    // cout << "maxCharging: " << maxCharging << "ev_b: " << ev_b << endl;
                }
                else
                {
                    z = false;
                }
                double hourly_laod = load_trace[index_t_load] + maxCharging;
                c = fmax(solar_trace[index_t_solar] * pv - hourly_laod, 0);
                d = fmax(hourly_laod - solar_trace[index_t_solar] * pv, 0);
                max_c = fmin(calc_max_charging(c, b), alpha_c);
                max_d = fmin(calc_max_discharging(d, b), alpha_d);

                // cout << "BEFORE c: " << c << "d: " << d << "max_c: " << max_c << "max_d: " << max_d << "ev_b: " << ev_b << "b : " << b << endl;

                operationResult = unidirectional_hybrid(b, ev_b, c, d, z, max_c, max_d, maxCharging, is_home, hour);
                // cout << "AFTER ev_b: " << operationResult.first << "b: " << operationResult.second << endl;
            }
            else if (Operation_policy == "optimal_bidirectional")
            {
                // cout << "EV DAY: " << allDailyStatuses[ev_day][hour].dayNumber << "hour: " << hour << "nextDept: " << convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) << endl;
                // cout << "BEFORE c2: " << c2 << "d2: " << d2 << "max_c2: " << max_c2 << "max_d2: " << max_d2 << "ev_b: " << ev_b << "b : " << b << endl;
                total_load += load_trace[index_t_load];
                operationResult = bidirectional_optimal(b, ev_b, c2, d2, z, max_c2, max_d2, is_home, hour);
                // cout << "AFTER ev_b: " << operationResult.first << "b: " << operationResult.second << endl;
            }
            else if (Operation_policy == "safe_bidirectional")
            {
                // wir gucken jedes mal ob man ev chargen kann, und wenn ja, chargen wir.
                // nicht kurz vor departure dischargen
                // hier computen ob man ev chargen kann
                // cout << "EV DAY: " << allDailyStatuses[ev_day][hour].dayNumber << "hour: " << hour << "nextDept: " << convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) << endl;

                bool dont_discharge = false;
                if (convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) == hour + 1)
                {
                    // do not discharge if the EV is about to leave
                    dont_discharge = true;
                }
                if (is_home == true)
                {
                    maxCharging = get_maxCharging(ev_b);
                }
                else
                {
                    maxCharging = 0.0;
                }
                // cout << "maxCharging: " << maxCharging << "ev_b: " << ev_b <<endl;

                if (maxCharging > 0)
                {
                    z = true;
                }
                // charging load ist in c und d schon mit eingerechnet
                double hourly_laod = load_trace[index_t_load] + maxCharging;
                c = fmax(solar_trace[index_t_solar] * pv - hourly_laod, 0);
                d = fmax(hourly_laod - solar_trace[index_t_solar] * pv, 0);
                max_c = fmin(calc_max_charging(c, b), alpha_c);
                max_d = fmin(calc_max_discharging(d, b), alpha_d);
                cout << "BEFORE c: " << c << "d: " << d << "max_c: " << max_c << "max_d: " << max_d << "ev_b: " << ev_b << "b : " << b << endl;

                operationResult = maximise_solar_charging_safe(b, ev_b, c, d, max_c, max_d, is_home, dont_discharge, z, maxCharging, hour);
                // cout << "AFTER ev_b: " << operationResult.first << "b: " << operationResult.second << endl;
            }

            else if (Operation_policy == "hybrid_bidirectional")
            {
                int chargingHour = lastp(allDailyStatuses[ev_day], ev_b, hour);
                double maxCharging = 0.0;
                // cout << "EV DAY: " << allDailyStatuses[ev_day][hour].dayNumber << "hour: " << hour << "nextDept: " << convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) << endl;
                // cout << "chargingHour: " << chargingHour << "currentHour: " << hour << endl;
                if (chargingHour == hour)
                {
                    z = true;
                    maxCharging = get_maxCharging(ev_b);
                    // cout << "maxCharging: " << maxCharging << "ev_b: " << ev_b <<endl;
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
                // solar_trace[index_t_solar] = 0;
                c = fmax(solar_trace[index_t_solar] * pv - hourly_laod, 0);
                // c = 0; for W+E and E sceanrios
                d = fmax(hourly_laod - solar_trace[index_t_solar] * pv, 0);
                max_c = fmin(calc_max_charging(c, b), alpha_c);
                max_d = fmin(calc_max_discharging(d, b), alpha_d);
                // cout << "BEFORE c: " << c << "d: " << d << "max_c: " << max_c << "max_d: " << max_d << "ev_b: " << ev_b << "b : " <<b << endl;

                operationResult = maximise_solar_charging(b, ev_b, c, d, z, max_c, max_d, maxCharging, is_home, hour);
                // cout << "AFTER ev_b: " << operationResult.first << "b: " << operationResult.second<< endl;
            }
            else
            {
                std::cout << "ERROR: Invalid operation policy selected" << std::endl;
            }
            // cout << "ev_b before update: " << ev_b << "b before update" << b << endl;

            ev_b = operationResult.first;
            b = operationResult.second;
            // cout << "ev_b after update: " << ev_b << "b after update" << b << endl;
            // cout << "load_deficit: " << load_deficit << "load_sum" << load_sum << endl;
            //  Update current SOC in EVStatus and carry over to next hour
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

            // cout << "OK" << endl;
            // cout << "hour: " << hour << "day" << day << endl;
            // cout << "ev status: " << allDailyStatuses[EV_index][hour].isAtHome << endl;
            // cout << "next departure" << convertTimeToHour(allDailyStatuses[EV_index][hour].nextDepartureTime) << endl;
            if (convertTimeToHour(allDailyStatuses[ev_day][hour].nextDepartureTime) == hour + 1)
            {
                // cout << "about to push soc value: " << last_soc << endl;
                socValues.push_back(last_soc);
                // cout << "push was successful " << endl;
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

    // first, find the lowest value of cells that will get us epsilon loss when the PV is maximized
    // use binary search
    double cells_U = cells_max;
    double cells_L = cells_min;
    double mid_cells = 0.0;
    double loss = 0.0;

    // set the starting number of battery cells to be the upper limit that was converged on
    double starting_cells = cells_U;
    double starting_cost = B_inv * starting_cells + PV_inv * pv_max;
    double lowest_feasible_pv = pv_max;

    double lowest_cost = starting_cost;
    double lowest_B = starting_cells * kWh_in_one_cell;
    double lowest_C = pv_max;
    vector<SimulationResult> curve;

    loss = sim(load_trace, solar_trace, start_index, end_index, 0, 4, b_0, evRecords, allDailyStatuses, max_soc, min_soc, Ev_start);
}