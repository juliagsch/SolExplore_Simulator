# This file contains the code used to fetch the faraday raw data. We iterate through all of the days of the year and 
# save the load trace for a given population on a given day. The daily load traces are saved in ./policy_data/faraday_raw. Using the file ./policy_data/prepare_inputs.py, the data can be aggregated
# to create a yearly load trace for each household in the population. While the daily traces in faraday_raw contain 700 households living in any housing type, 
# we only use the 300 households living in terraced, detached or semi-detached houses. The remaining 700 households were fetched for a different project
# and can be ignored.
# Please request a Faraday API key here: https://developer.nrel.gov/signup/ and specify it in your .env file to run this script.

import requests
import os
import numpy as np
import json

FARADAY_KEY = os.getenv("FARADAY_KEY")
url = "https://faraday-api-gateway-28g4j071.nw.gateway.dev/v4/predict/"

days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
months = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"]
days_per_month = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

headers = {
    "accept": "application/json",
    "content-type": "application/json",
    "x-api-key": FARADAY_KEY
}

def get_payload(day, month):
    return {
        "day_of_week": day,
        "month_of_year": month,
        "population": [
            # {
            #     "name": "NoLCT",
            #     "count": 500,
            #     "attributes": {
            #         "energy_rating": "Any",
            #         "urbanity": "Any",
            #         "property_type": "Any House Types",
            #         "is_mains_gas": "Any",
            #         "lct": ["Has No LCTs"],
            #         "tariff_type": "any"
            #     }
            # },
            # {
            #     "name": "EV",
            #     "count": 200,
            #     "attributes": {
            #         "energy_rating": "Any",
            #         "urbanity": "Any",
            #         "property_type": "Any House Types",
            #         "is_mains_gas": "Any",
            #         "lct": ["Has Electric Vehicles"],
            #         "tariff_type": "any"
            #     }
            # },
            {
                "name": "DetachedA",
                "count": 59,
                "attributes": {
                "energy_rating": "A/B/C",
                "urbanity": "Urban",
                "property_type": "Detached",
                "is_mains_gas": "Has Mains Gas",
                "lct": [
                "Has No LCTs"
                ],
                "tariff_type": "any"
                }
            },
            {
                "name": "DetachedD",
                "count": 41,
                "attributes": {
                "energy_rating": "D/E",
                "urbanity": "Urban",
                "property_type": "Detached",
                "is_mains_gas": "Has Mains Gas",
                "lct": [
                "Has No LCTs"
                ],
                "tariff_type": "any"
                }
            },
            {
                "name": "TerracedA",
                "count": 59,
                "attributes": {
                "energy_rating": "A/B/C",
                "urbanity": "Urban",
                "property_type": "Terraced",
                "is_mains_gas": "Has Mains Gas",
                "lct": [
                "Has No LCTs"
                ],
                "tariff_type": "any"
                }
            },
            {
                "name": "TerracedD",
                "count": 41,
                "attributes": {
                "energy_rating": "D/E",
                "urbanity": "Urban",
                "property_type": "Terraced",
                "is_mains_gas": "Has Mains Gas",
                "lct": [
                "Has No LCTs"
                ],
                "tariff_type": "any"
                }
            },
            {
                "name": "Semi-detachedA",
                "count": 59,
                "attributes": {
                "energy_rating": "A/B/C",
                "urbanity": "Urban",
                "property_type": "Semi-detached",
                "is_mains_gas": "Has Mains Gas",
                "lct": [
                "Has No LCTs"
                ],
                "tariff_type": "any"
                }
            },
            {
                "name": "Semi-detachedD",
                "count": 41,
                "attributes": {
                "energy_rating": "D/E",
                "urbanity": "Urban",
                "property_type": "Semi-detached",
                "is_mains_gas": "Has Mains Gas",
                "lct": [
                "Has No LCTs"
                ],
                "tariff_type": "any"
                }
            },
        ]
    }

day_idx = 0
count = 0

for month_idx, total_month_days in enumerate(days_per_month):
    month_day = 0
    while month_day<total_month_days:
        print(f"Fetching profiles for {days[day_idx]} in {months[month_idx]}. Current day of the year: {count}")
        payload = get_payload(days[day_idx], months[month_idx])
        response = requests.post(url, json=payload, headers=headers)
        if response.status_code == 200:
            data = response.json()

            with open(f"./policy_data/faraday_raw/day_{count}.json", "w") as f:
                json.dump(data, f, indent=4)

            day_idx = (day_idx + 1)%7
            count += 1
            month_day += 1
        else: 
            print(f"Error: {response.status_code}, {response.text}")

print(f"Fetched profiles for {count} days")