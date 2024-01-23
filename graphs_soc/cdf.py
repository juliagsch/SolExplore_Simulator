import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import gaussian_kde
from scipy.integrate import cumtrapz

# Assuming `soc_values` is a numpy array of SOC values at departure
soc_values = np.array([...])  # Replace with your SOC values

# Compute Kernel Density Estimation (KDE)
kde = gaussian_kde(soc_values)

# Create a range of values for which to evaluate the PDF
soc_range = np.linspace(min(soc_values), max(soc_values), 1000)
pdf_values = kde(soc_range)

# Compute the CDF using the cumulative trapezoidal rule
cdf_values = cumtrapz(pdf_values, soc_range, initial=0)

# Plot the PDF and CDF
plt.figure(figsize=(12, 6))
plt.subplot(1, 2, 1)
plt.plot(soc_range, pdf_values, label='PDF')
plt.xlabel('SOC at Departure')
plt.ylabel('Probability Density')
plt.title('PDF of SOC at Departure')
plt.legend()

plt.subplot(1, 2, 2)
plt.plot(soc_range, cdf_values, label='CDF', color='orange')
plt.xlabel('SOC at Departure')
plt.ylabel('Cumulative Probability')
plt.title('CDF of SOC at Departure')
plt.legend()

plt.tight_layout()
plt.show()
