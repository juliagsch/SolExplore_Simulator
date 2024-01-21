import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file
df = pd.read_csv('evaluation_results.csv')

# Convert numeric columns to float
df['Cost'] = pd.to_numeric(df['Cost'], errors='coerce')
df['Battery'] = pd.to_numeric(df['Battery'], errors='coerce')
df['PV'] = pd.to_numeric(df['PV'], errors='coerce')

# Example 1: Bar chart of average cost for each EV Charging Policy
avg_cost_by_ev_policy = df.groupby('EV Charging Policy')['Cost'].mean()
avg_cost_by_ev_policy.plot(kind='bar', title='Average Cost by EV Charging Policy')
plt.ylabel('Average Cost')
plt.show()

# Example 2: Bar chart of average cost for each Operation Policy
avg_cost_by_op_policy = df.groupby('Operation Policy')['Cost'].mean()
avg_cost_by_op_policy.plot(kind='bar', title='Average Cost by Operation Policy')
plt.ylabel('Average Cost')
plt.show()

# Example 3: Scatter plot of Cost vs. Battery size for each WFH Type
for wfh_type in df['WFH Type'].unique():
    subset = df[df['WFH Type'] == wfh_type]
    plt.scatter(subset['Battery'], subset['Cost'], label=f'WFH {wfh_type}')
plt.xlabel('Battery Size')
plt.ylabel('Cost')
plt.title('Cost vs. Battery Size for each WFH Type')
plt.legend()
plt.show()
