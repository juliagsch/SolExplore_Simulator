import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# This file creates the aggregated scenarios graph with uni and bidirectional scenarios all in the same file
# Load data from CSV files
uni_best = pd.read_csv('1707_e_uni_best_all_scenarios.csv')
uni_worst = pd.read_csv('1707_e_uni_worst_all_scenarios.csv')
bi_best = pd.read_csv('1707_e_bi_best_all_scenarios.csv')
bi_worst = pd.read_csv('1707_e_bi_worst_all_scenarios.csv')

# Function to convert emission values from kilotonnes to megatonnes
def convert_to_megatonnes(df):
    df.iloc[:, 1:] = df.iloc[:, 1:] / 1000
    return df

# Convert all dataframes
uni_best = convert_to_megatonnes(uni_best)
uni_worst = convert_to_megatonnes(uni_worst)
bi_best = convert_to_megatonnes(bi_best)
bi_worst = convert_to_megatonnes(bi_worst)

# Function to plot the graph with simplified legend and adjusted x-axis
def plot_combined_graph(uni_best, uni_worst, bi_best, bi_worst, title):
    plt.figure(figsize=(12, 6))  # Increase figure size for better clarity
    uni_scenarios = uni_best.columns[1:]  # Extract scenario names from uni_best
    bi_scenarios = bi_best.columns[1:]
    #print(uni_scenarios)
    #print(bi_scenarios)
    palette = sns.color_palette("husl", len(set(uni_scenarios + bi_scenarios)))  # Define color palette

    combined_scenarios = list(set(uni_scenarios)) 

    for i, scenario in enumerate(combined_scenarios):
        color = palette[i]
        print(scenario + "----------------")

        if scenario == 'H+P':
            print("H+P")
            # Plot combined H+P scenario lines in a single color
            sns.lineplot(x='Conversion Rate (%)', y='H+P', data=uni_best, 
                         color=color, marker='o', linestyle='-')
            sns.lineplot(x='Conversion Rate (%)', y='H+P', data=uni_worst, 
                         color=color, marker='o', linestyle='--')
            plt.fill_between(x=uni_best['Conversion Rate (%)'], y1=uni_best['H+P'],
                             y2=uni_worst['H+P'], color=color, alpha=0.3)
            # Add combined label
            max_emission_best = uni_best.loc[uni_best['Conversion Rate (%)'] == uni_best['Conversion Rate (%)'].max(), 'H+P'].values[0]
            plt.text(105, max_emission_best, 'H + P (Uni & Bi)', color=color, ha='left', va='center')
        else:
            if scenario in uni_scenarios:
                label = scenario
                if scenario == 'H+E+P':
                    label = 'H + E + P (Uni)'
                if scenario == 'H+E':
                    label = 'H + E (Uni)'
                if scenario == 'E+P':
                    label = 'E + P (Uni)'
                

                # Plot unidirectional best scenario lines
                sns.lineplot(x='Conversion Rate (%)', y=scenario, data=uni_best, 
                             color=color, marker='o', linestyle='-')
                # Plot unidirectional worst scenario lines
                sns.lineplot(x='Conversion Rate (%)', y=scenario, data=uni_worst, 
                             color=color, marker='o', linestyle='--')
                plt.fill_between(x=uni_best['Conversion Rate (%)'], y1=uni_best[scenario],
                                 y2=uni_worst[scenario], color=color, alpha=0.3)
                # Adding scenario labels to the right of the lines
                max_emission_best = uni_best.loc[uni_best['Conversion Rate (%)'] == uni_best['Conversion Rate (%)'].max(), scenario].values[0]
                plt.text(105, max_emission_best, label, color=color, ha='left', va='center')

            if scenario in bi_scenarios:
                label = scenario
                if scenario == 'H+E+P':
                    label = 'H + E + P (Bi)'
                elif scenario == 'H+E':
                    label = 'H + E (Bi)'
                if scenario == 'E+P':
                    label = 'E + P (Bi)'

                # Plot bidirectional best scenario lines
                sns.lineplot(x='Conversion Rate (%)', y=scenario, data=bi_best, 
                             color=color, marker='o', linestyle='-')
                # Plot bidirectional worst scenario lines
                sns.lineplot(x='Conversion Rate (%)', y=scenario, data=bi_worst, 
                             color=color, marker='o', linestyle='--')
                plt.fill_between(x=bi_best['Conversion Rate (%)'], y1=bi_best[scenario],
                                 y2=bi_worst[scenario], color=color, alpha=0.3)
                # Adding scenario labels to the right of the lines
                max_emission_best = bi_best.loc[bi_best['Conversion Rate (%)'] == bi_best['Conversion Rate (%)'].max(), scenario].values[0]
                plt.text(105, max_emission_best, label, color=color, ha='left', va='center')

    plt.title(title)
    plt.xlabel('Conversion Rate (%)')
    plt.ylabel('Total CO2 Emissions (Megatonnes)')
    plt.xlim([0, 125])  # Extend x-axis to 125 to fit labels
    plt.ylim([6, 20])  # Adjust according to your data range (6 to 20 megatonnes)
    plt.xticks(range(0, 101, 10))  # Ensure ticks are only up to 100
    
    plt.show()


# Plotting the graphs for unidirectional and bidirectional cases
plot_combined_graph(uni_best, uni_worst, bi_best, bi_worst, 'Unidirectional vs Bidirectional Case: CO2 Emissions vs. Conversion Rate')

