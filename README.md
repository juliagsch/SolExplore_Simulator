# Robust Sizing

### To compile all three programs

```
make
```

### To run programs

Simulation method for sizing:

```
./bin/sim pv_cost battery_cost pv_max battery_max metric epsilon conf n_days load pv ev_max ev_min ev_battery charging_rate op ev lim
```

Where:

`pv_cost`: price per kW of PV panels

`battery_cost`: price per kWh of battery

`pv_max`: max number of PV panels available, in terms of kW

`battery_max`: max number of battery available, in terms of kWh

`metric`: 0 for LOLP metric, 1 for EUE metric

`epsilon`: epsilon (for LOLP target) or theta (for EUE target) value

`conf`: confidence

`n_days`: number of days in each sample

`load`: name of load file, values in kW (for example, see load.txt)

`pv`: name of pv generation file, values in kW (for example, see pv.txt)

`ev_max`: upper threshold to which the EV battery should be charged

`ev_min`: lower threshold or EV battery

`ev_battery`: battery capacity of EV in kWh

`charging_rate`: charging rate of EV, determines how many kWh can be charged per hour

`op`: operational policy in [no_ev, safe_arrival, safe_departure, arrival_limit, bidirectional]

`ev`: name of ev trace file

`lim`: if operational policy == arrival limit, the EV battery should be charged to `lim` at arrival

Example command:

```
./bin/sim 1250 460 70 225 1 0.2 0.8 365 load.txt solar.txt 0.8 0.2 60 7.4 bidirectional ev.csv 20
```
