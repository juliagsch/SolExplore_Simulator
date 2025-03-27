"""Tests are written for the case when we have both PV and EV. We have a 4kW PV system without a stationary battery."""
import os
import pytest
import subprocess
from test_utils import extract_result

test_data_path = os.path.abspath("./tests/test_data")
eta_ev = 0.935 # Charging and discharging efficiency of the EV

def test_unidirectional():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op = "safe_unidirectional"
    num_days = 1
    command = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 2
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    assert result["total_load"] == result["grid_import"] + 11 # We have 10 hours when we can cover the load using solar. Only in one of those hours, the EV is charging. 
    assert result["ev_power_charged"] == pytest.approx(result["ev_power_used"]+result["ev_battery_diff"]+result["power_lost"], 1e-6)
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["ev_battery_diff"]+result["power_lost"], 1e-6)
    assert result["total_cost"] == pytest.approx((result["ev_power_charged"]-1)*0.07 + 9*0.35 + 5*0.07, 1e-6)
    assert result["power_lost"] == pytest.approx(result["ev_power_charged"]-result["ev_power_charged"]*eta_ev, 1e-6)

def test_bidirectional():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op = "hybrid_bidirectional"
    num_days = 1
    command = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)
    print(command)
    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 2
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    # Addition can lead to small rounding error, therefore we need to approximate.
    assert result["total_load"] == pytest.approx(result["grid_import"]+10+5.333333, 1e-6) # EV is charged by 5.333333 kWh of solar power
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["power_lost"]+result["ev_battery_diff"], 1e-6)
    assert result["total_cost"] == pytest.approx((result["ev_power_charged"]-5.333333)*0.07 + 1*0.35, 1e-6) # We can always cover the household load from the EV battery or PV except during 1h (at 8am)
    assert result["power_lost"] == pytest.approx((result["ev_power_charged"])*(1-eta_ev)+(13*((1.0 / eta_ev)-1)), 1e-6) # We discharge 13 kWh from the EV battery

def test_bidirectional_two_trips():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev2.csv"
    op = "hybrid_bidirectional"
    num_days = 1
    command = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op} {ev_file_path} 0 4"
    result = subprocess.run(command.split(), stdout=subprocess.PIPE, text=True)
    print(command)
    # Extract numbers from output
    result = extract_result(result.stdout)
    
    assert result["ev_power_used"] == 6
    assert result["household_load"] == 24
    assert result["total_hours"] == 24
    # Addition can lead to small rounding error, therefore we need to approximate.
    assert result["total_load"] == pytest.approx(result["grid_import"]+10+5.333333, 1e-6) # EV is charged by 5.333333 kWh of solar power
    assert result["total_load"] == pytest.approx(result["household_load"]+result["ev_power_used"]+result["power_lost"]+result["ev_battery_diff"], 1e-6)
    assert result["total_cost"] == pytest.approx((result["ev_power_charged"]-5.333333)*0.07 + 3*0.35, 1e-6) # We can always cover the household load from the EV or PV except during 3h (1h before sunrise and 2h after sunset as the EV is away or charging)
    assert result["power_lost"] == pytest.approx((result["ev_power_charged"])*(1-eta_ev)+(11*((1.0 / eta_ev)-1)), 1e-6) # We discharge 11 kWh from the EV battery

def test_uni_vs_bi():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op_uni = "safe_unidirectional"
    op_bi = "hybrid_bidirectional"
    num_days = 1
    command_uni = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_uni} {ev_file_path} 0 4"
    command_bi = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_bi} {ev_file_path} 0 4"

    result_uni = subprocess.run(command_uni.split(), stdout=subprocess.PIPE, text=True)
    result_bi = subprocess.run(command_bi.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result_uni = extract_result(result_uni.stdout)
    result_bi = extract_result(result_bi.stdout)

    assert result_uni["ev_power_used"] == 2
    assert result_uni["household_load"] == 24
    assert result_uni["ev_power_used"] == result_bi["ev_power_used"]
    assert result_uni["household_load"] == result_bi["household_load"]
    assert result_uni["grid_import"] > result_bi["grid_import"]
    assert result_uni["power_lost"] < result_bi["power_lost"]
    assert result_uni["ev_power_charged"] < result_bi["ev_power_charged"]

def test_pv_vs_nopv():
    house_file_path = f"{test_data_path}/house1.txt"
    solar_file_path = f"{test_data_path}/solar1.txt"
    ev_file_path = f"{test_data_path}/ev1.csv"
    op_bi = "hybrid_bidirectional"
    num_days = 1
    command_nopv = f"./bin_nopvbutev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_bi} {ev_file_path} 0 4"
    command_pv = f"./bin_pvandev/sim 2100 480 10 20 1 0.5 0.95 {num_days} {house_file_path} {solar_file_path} 0.8 0.2 60.0 7.4 {op_bi} {ev_file_path} 0 4"

    result_nopv = subprocess.run(command_nopv.split(), stdout=subprocess.PIPE, text=True)
    result_pv = subprocess.run(command_pv.split(), stdout=subprocess.PIPE, text=True)

    # Extract numbers from output
    result_nopv = extract_result(result_nopv.stdout)
    result_pv = extract_result(result_pv.stdout)

    assert result_nopv["ev_power_used"] == 2
    assert result_nopv["household_load"] == 24
    assert result_nopv["ev_power_used"] == result_pv["ev_power_used"]
    assert result_nopv["household_load"] == result_pv["household_load"]
    assert result_nopv["grid_import"] > result_pv["grid_import"]
    assert result_nopv["power_lost"] > result_pv["power_lost"] # We are discharging more without pv
