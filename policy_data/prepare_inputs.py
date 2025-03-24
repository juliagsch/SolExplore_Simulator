import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import json
import shutil
import random

def process_responses(input_file):
    print(input_file)
    # Load JSON file
    with open(input_file, 'r') as file:
        data = json.load(file)

    population = {
        "NoLCT": "NoLCT",
        "EV": "EV",
        "DetachedA": "Detached",
        "DetachedD": "Detached",
        "TerracedA": "Terraced",
        "TerracedD": "Terraced",
        "Semi-detachedA": "Semi-detached",
        "Semi-detachedD": "Semi-detached"
    }

    for p in population:
        os.makedirs(f"./policy_data/processed_large/{population[p]}", exist_ok=True)

        # Extract results
        response = next((item for item in data["message"]["results"] if item["name"] == p), None)
        kwh = response["kwh"]

        # Write to output text file with each hourly value on a new line
        for idx, daily_trace in enumerate(kwh):
            with open(f'./policy_data/processed_large/{population[p]}/{p}_{idx}.txt', 'a') as file:
                for i in range(0, len(daily_trace) - 1, 2):
                    # Get hourly values by adding half-hourly load
                    file.write(f"{float(daily_trace[i]) + float(daily_trace[i + 1])}\n")


def read_and_process_hourly(file_path):
    with open(file_path, 'r') as file:
        # Read all lines and convert them to floats
        data = np.array([float(line.strip()) for line in file.readlines()])
        
    # Calculate total PV generation
    total_pv_generation = np.sum(data)
    
    # Calculate average hourly production
    hourly_averages = [np.mean(data[hour::24]) for hour in range(24)]
    
    return total_pv_generation, hourly_averages

def read_and_process_weekdays(file_path):
    with open(file_path, 'r') as file:
        # Read all lines and convert them to floats
        data = np.array([float(line.strip()) for line in file.readlines()])

    weekday_total = [0.0 for i in range(7)]
    for day in range(365):
        weekday_data = data[day*24:day*24+24]
        weekday_total[day%7] += np.sum(weekday_data)
    
    return weekday_total

def read_and_process_monthly(file_path):
    with open(file_path, 'r') as file:
        # Read all lines and convert them to floats
        data = np.array([float(line.strip()) for line in file.readlines()])
        
    # Calculate total PV generation
    total_pv_generation = np.sum(data)
    
    # Assume data has 365 days, so we split it into 12 months
    days_in_month = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
    
    monthly_averages = []
    start_idx = 0
    for days in days_in_month:
        end_idx = start_idx + days
        monthly_averages.append(np.sum(data[start_idx * 24:end_idx * 24]))  # Average over the month
        start_idx = end_idx  # Move to the next month
    
    return total_pv_generation, monthly_averages

def analyze_and_plot_daily(load_files):
    plt.figure(figsize=(10, 5))
    load_files = random.sample(load_files, 10)

    for load_file in load_files:
        _, hourly = read_and_process_hourly(load_file)
        # label = f"NoEV_{load_file}" if load_file < 50 else f"EV_{load_file}"
        label = load_file
        plt.plot(range(24), hourly, label=label, marker='o')
    

    plt.title('Average Hourly Load')
    plt.xlabel('Hour of the Day')
    plt.ylabel('Average Load')
    plt.xticks(range(24), labels=[f'{hour}:00' for hour in range(24)], rotation=45)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig('./policy_data/average_daily_load.png')
    plt.show()

def analyze_and_plot_weekly(load_files):
    plt.figure(figsize=(12, 6))
    for load_file in load_files:
        weekly = read_and_process_weekdays(f'./policy_data/processed/{load_file}.txt')
        plt.plot(range(7), weekly, label=load_file, marker='o')
    
    plt.title('Weekly Load Average')
    plt.xlabel('Weekday')
    plt.ylabel('Load')
    plt.xticks(range(7), labels=[f'{month}' for month in ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]], rotation=45)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig('./policy_data/average_weekly_load.png')
    plt.show()

def analyze_and_plot_monthly(load_files):
    plt.figure(figsize=(12, 6))
    total = [0.0 for _ in range(12)]
    for load_file in load_files:
        _, monthly = read_and_process_monthly(f'./policy_data/processed/{load_file}.txt')
        total = np.add(total, monthly)
    
    plt.plot(range(12), total, label="average", marker='o')
    
    plt.title('Monthly Load')
    plt.xlabel('Month of the Year')
    plt.ylabel('Load')
    plt.xticks(range(12), labels=[f'{month}' for month in ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]], rotation=45)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig('./policy_data/average_monthly_load.png')
    plt.show()

def get_load_files():
    files = []
    for type in ["Detached", "Semi_Detached", "Terraced"]:
        files = files + [f"./policy_data/{type}/{f}" for f in os.listdir(f"./policy_data/{type}") if f.endswith('.txt')]
    return files

if __name__ == "__main__":
    # Create output directory
    # shutil.rmtree("./policy_data/processed_large/")
    # os.makedirs('./policy_data/processed_large/', exist_ok=True)

    # # First process the raw data
    # for load_file_idx in range(365):
    #     process_responses(f'./policy_data/faraday_large/day_{load_file_idx}.json')
        
    # Then analyze and plot the processed data
    files = get_load_files()
    analyze_and_plot_daily(files)
    # analyze_and_plot_weekly([i for i in range(1,50,5)])
    # analyze_and_plot_monthly([i for i in range(50,75,1)])
