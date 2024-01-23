import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Load the data from the CSV file
file_path = 'averaged_evaluation_results_10_0.1.csv'  # Replace with your CSV file path
data = pd.read_csv(file_path)

# Filter only unidirectional policies
#data = data[data['Operation Policy'].str.contains("unidirectional")]

# Create a seaborn multilevel bar chart
plt.figure(figsize=(12, 8))
bar_plot = sns.barplot(x='Operation Policy', y='Cost', hue='WFH Type', data=data, palette="rocket")
sns.color_palette("flare", as_cmap=True)

# Additional customization
plt.title('Average Cost by Operation Policy and WFH Type')
plt.ylabel('Average Cost')
plt.xlabel('Operation Policy')
plt.xticks(rotation=45)
plt.legend(title='WFH Type')

# Display the cost values above the bars
for p in bar_plot.patches:
    bar_plot.annotate(f'{int(p.get_height())}', 
                      (p.get_x() + p.get_width() / 2., p.get_height()), 
                      ha='center', va='center', 
                      xytext=(0, 9), 
                      textcoords='offset points')

# Display the plot
plt.tight_layout()
plt.show()
