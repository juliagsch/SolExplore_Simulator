import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Load data from CSV files
uni_best = pd.read_csv('e_uni_best.csv')
uni_worst = pd.read_csv('e_uni_worst.csv')
bi_best = pd.read_csv('e_bi_best.csv')
bi_worst = pd.read_csv('e_bi_worst.csv')

# Function to convert emission values from kilotonnes to megatonnes
def convert_to_megatonnes(df):
    df.iloc[:, 1:] = df.iloc[:, 1:] / 1000
    return df

# Convert all dataframes
uni_best = convert_to_megatonnes(uni_best)
uni_worst = convert_to_megatonnes(uni_worst)
bi_best = convert_to_megatonnes(bi_best)
bi_worst = convert_to_megatonnes(bi_worst)

# Function to plot the graph with improved annotations
def plot_graph(df_best, df_worst, title):
    plt.figure(figsize=(10, 6))
    # Define two shades of green
    green_palette = ["#2ca02c", "#98df8a"]

    # Plotting best and worst scenario lines
    sns.lineplot(x='Conversion Rate (%)', y='Total Emissions (Mega Tonnes)', data=df_best, 
                 label='Best Scenario', marker='o', color=green_palette[0])
    sns.lineplot(x='Conversion Rate (%)', y='Total Emissions (Mega Tonnes)', data=df_worst, 
                 label='Worst Scenario', marker='o', color=green_palette[1])
    
    # Filling the area between the lines
    plt.fill_between(x=df_best['Conversion Rate (%)'], y1=df_best['Total Emissions (Mega Tonnes)'],
                     y2=df_worst['Total Emissions (Mega Tonnes)'], color='gray', alpha=0.3)

    # Adding annotations for selected conversion rates with adjusted distances
    rates = [0, 20, 40, 60, 80, 100]
    vertical_spacing = 0.3  # Adjusted for megatonnes scale
    for rate in rates:
        best_emission = round(df_best.loc[df_best['Conversion Rate (%)'] == rate, 'Total Emissions (Mega Tonnes)'].values[0])
        worst_emission = round(df_worst.loc[df_worst['Conversion Rate (%)'] == rate, 'Total Emissions (Mega Tonnes)'].values[0])
        plt.text(rate, best_emission - vertical_spacing, f'{best_emission}', color=green_palette[0], va='top', ha='center', fontsize=9)
        plt.text(rate, worst_emission + vertical_spacing, f'{worst_emission}', color=green_palette[1], va='bottom', ha='center', fontsize=9)

    # plt.title(title)
    plt.xlabel('Conversion Rate (%)')
    plt.ylabel('Total CO2 Emissions (Mega Tonnes)')
    plt.ylim([6, 15])  # Specific y-axis limits for megatonnes
    plt.legend()
    plt.show()

# Plotting the graphs for unidirectional and bidirectional cases
plot_graph(uni_best, uni_worst, 'Unidirectional Case: CO2 Emissions vs. Conversion Rate')
plot_graph(bi_best, bi_worst, 'Bidirectional Case: CO2 Emissions vs. Conversion Rate')
