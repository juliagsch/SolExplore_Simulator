// chebyschev bound code

#include <cmath>
#include <iostream>
#include <fstream>
#include "cheby.h"

using namespace std;

// Function to find mean.
double mean(vector<double> &values)
{

	double n = values.size();
	double sum = 0.0;
	for (int i = 0; i < n; i++)
	{
		sum = sum + values[i];
	}
	return sum / n;
}

double std_dev(vector<double> &values, double mean)
{

	double n = values.size();
	double std_dev = 0.0;

	for (int i = 0; i < n; i++)
	{
		std_dev += pow(values[i] - mean, 2);
	}

	return sqrt(std_dev / n);
}

double interpolate(vector<double> &xData, vector<double> &yData, double x)
{
	int size = xData.size();

	// find value of i such that x lies between points at indices i and i+1 of xData
	for (int i = 0; i < size - 1; i++)
	{
		if ((xData[i] <= x && x <= xData[i + 1]) || (xData[i + 1] <= x && x <= xData[i]))
		{
			double x1 = xData[i];
			double y1 = yData[i];
			double x2 = xData[i + 1];
			double y2 = yData[i + 1];

			if (x1 == x2)
			{
				return y1;
			}

			// gradient
			double dydx = (y2 - y1) / (x2 - x1);

			// linear interpolation
			return y2 + dydx * (x - x1);
		}
	}

	return -1;
}

double calculate_sample_lambda(int N, double bound)
{
	for (double l = (sqrt(1.0 / bound)); l <= N; l += 0.01)
	{
		double a = (1.0 / (N + 1.0));
		double b = (N + 1) * ((pow(N, 2)) - 1 + N * (pow(l, 2)));
		double c = pow(N, 2) * pow(l, 2);

		if (a * floor(b / c) < bound)
		{
			return l;
		}
	}

	return 0.0;
}

vector<vector<double>>
chebyshev(vector<vector<double>> &X_vals, vector<vector<double>> &Y_vals, double confidence, double step,
		  double max_val)
{

	int n = X_vals.size();

	double lambda = calculate_sample_lambda(n, 1 - confidence);

	vector<double> cheby_X;
	vector<double> cheby_Y;

	for (double X_val = 0.0; X_val <= max_val; X_val += step)
	{

		vector<double> Y_set;

		// find value of Y at X_val for every curve
		for (int trace = 0; trace < X_vals.size(); trace++)
		{
			double interpolated_Y = interpolate(X_vals[trace], Y_vals[trace], X_val);

			if (interpolated_Y >= 0)
			{
				Y_set.push_back(interpolated_Y);
			}
			else
			{
				// as soon as a single curve is not added, break out of the loop because we dont care about
				// these values anymore.
				// cout << "trace " << trace << " didnt have a value for x=" << X_val << endl;
				break;
			}
		}

		// count it only if every curve had an interpolation value for X_val
		if (Y_set.size() == X_vals.size())
		{
			double Y_mean = mean(Y_set);
			double Y_std = std_dev(Y_set, Y_mean);

			cheby_X.push_back(X_val);
			cheby_Y.push_back(lambda * Y_std + Y_mean);
			// cout << X_val << " " << lambda*Y_std + Y_mean << endl;
		}
	}

	vector<vector<double>> return_vector(2);
	return_vector[0] = cheby_X;
	return_vector[1] = cheby_Y;

	return return_vector;
}

SimulationResult
calculate_sample_bound(vector<vector<SimulationResult>> &sizing_curves, double epsilon, double confidence)
{

	int n = sizing_curves.size();

#ifdef DEBUG
	cout << "calculate_sample_bound: sizing_curve.size() = " << n << ", epsilon = " << epsilon << ", confidence = " << confidence << endl;
#endif

	// create arrays for all B and C values
	vector<vector<double>> B_values(n);
	vector<vector<double>> C_values(n);

	for (int i = 0; i < n; ++i)
	{
		int n_curve = sizing_curves[i].size();
		vector<double> Bs;
		vector<double> Cs;

		double last_B = -1, last_C = -1;

		for (auto &sim_result : sizing_curves[i])
		{
			if (sim_result.B != last_B || sim_result.C != last_C)
			{
				Bs.push_back(sim_result.B);
				Cs.push_back(sim_result.C);
				last_B = sim_result.B;
				last_C = sim_result.C;
			}
		}

		B_values[i] = Bs;
		C_values[i] = Cs;
	}

	vector<vector<double>> cheby_on_B = chebyshev(C_values, B_values, confidence, pv_step, pv_max);
	vector<vector<double>> cheby_on_C = chebyshev(B_values, C_values, confidence, cells_step * kWh_in_one_cell,
												  cells_max * kWh_in_one_cell);

#ifdef DEBUG
	// print chebyshev curves to files.
	cout << "DEBUG: cheby_on_C" << endl;
	cout << cheby_on_C[0].size() << endl;
	for (int i = 0; i < cheby_on_C[0].size(); i++)
	{
		cout << cheby_on_C[0][i] << "\t" << cheby_on_C[1][i] << endl;
	}

	cout << "DEBUG: cheby_on_B" << endl;
	cout << cheby_on_B[0].size() << endl;
	for (int i = 0; i < cheby_on_B[0].size(); i++)
	{
		cout << cheby_on_B[1][i] << "\t" << cheby_on_B[0][i] << endl;
	}
#endif

	// search the upper envelope for the cheapest system
	double lowest_cost = INFTY;
	double lowest_B = INFTY;
	double lowest_C = INFTY;

	for (double B_val = 0.0; B_val <= cells_max * kWh_in_one_cell; B_val += cells_step * kWh_in_one_cell)
	{

		double C1 = interpolate(cheby_on_B[1], cheby_on_B[0], B_val);
		double C2 = interpolate(cheby_on_C[0], cheby_on_C[1], B_val);

		if (C1 < 0 || C2 < 0)
		{
			continue;
		}

		double C_max = fmax(C1, C2);
		// ensure this value is on the search grid by rounding up to the nearest pv_step value
		if (fmod(C_max, pv_step) != 0)
		{
			C_max = C_max - fmod(C_max, pv_step) + pv_step;
		}

		double cost = B_inv * (B_val / kWh_in_one_cell) + PV_inv * C_max;
		if (cost < lowest_cost)
		{
			lowest_cost = cost;
			lowest_B = B_val;
			lowest_C = C_max;
		}
	}

	return SimulationResult(lowest_B, lowest_C, lowest_cost);
}
