"""Tests are written for the case when we have PV but no EV. We have a 4kW PV system without a stationary battery."""
import os
import pytest
import subprocess
from test_utils import extract_result

test_data_path = os.path.abspath("./tests/test_data")
eta_ev = 0.935 # Charging and discharging efficiency of the EV

def test_no_ev():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    num_days = 1
    command = f"./bin_withpvnoev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 no_ev {ev_file_path} 0 4"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 0
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] == 24
    assert result["grid_import"] == 14
    assert result["ev_power_charged"] == 0
    assert result["total_cost"] == pytest.approx(9*0.35 + 5*0.07, 1e-6)
    assert result["power_lost"] == 0

def test_ev_vs_noev():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op_bi = "hybrid_bidirectional"
    num_days = 1
    command_no_ev = f"./bin_withpvnoev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 no_ev {ev_file_path} 0 4"
    command_ev = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_bi} {ev_file_path} 0 4"

    command_no_ev = subprocess.run(command_no_ev.split(), stdout=subprocess.PIPE, text=True)
    command_ev = subprocess.run(command_ev.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    command_no_ev = extract_result(command_no_ev.stdout)
    command_ev = extract_result(command_ev.stdout)

    assert command_no_ev["household_load"] == 24
    assert command_no_ev["total_load"] < command_ev["total_load"]
    assert command_no_ev["grid_import"] > command_ev["grid_import"]-(command_ev["ev_battery_diff"]/eta_ev)
