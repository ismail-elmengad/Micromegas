import json
import numpy as np
import matplotlib.pyplot as plt


import matplotlib.pyplot as plt

# Loop through the serialized json file
with open("channel_performance.json") as json_file:
    data = json.load(json_file)

# Create figures with 4 rows and 4 columns of subplots
fig0, axs0 = plt.subplots(nrows=4, ncols=4, figsize=(16, 12))
fig1, axs1 = plt.subplots(nrows=4, ncols=4, figsize=(16, 12))
fig2, axs2 = plt.subplots(nrows=4, ncols=4, figsize=(16, 12))

# Flatten the axs arrays to iterate over each subplot
axs0 = axs0.flatten()
axs1 = axs1.flatten()
axs2 = axs2.flatten()

# Loop through the sectors and create pie charts for each sector
for i, (ax0, ax1, ax2) in enumerate(zip(axs0, axs1, axs2), start=1):
    unmasked_with_hits = 0
    unmasked_without_hits = 0
    masked_with_hits = 0
    masked_without_hits = 0
    # Loop through MMFE8s
    for key in data:
        # Loop through vmms
        for vmm in data[key]:
            # Loop through channels
            for channel in data[key][vmm]:
                if (channel[0] == 0 and channel[i] == 0):
                    unmasked_without_hits += 1
                elif (channel[0] == 0 and channel[i] == 1):
                    unmasked_with_hits += 1
                elif (channel[0] == 1 and channel[i] == 0):
                    masked_without_hits += 1
                elif (channel[0] == 1 and channel[i] == 1):
                    masked_with_hits += 1

    # Get various fractions of masking/hit statistics
    total_channels = unmasked_with_hits + unmasked_without_hits + masked_with_hits + masked_without_hits
    masked_channels = masked_with_hits + masked_without_hits
    umasked_channels = total_channels - masked_channels

    # Create pie charts for all four categories
    sizes = [masked_with_hits / total_channels, masked_without_hits / total_channels,
             unmasked_with_hits / total_channels, unmasked_without_hits / total_channels]
    labels = ['Masked w/ hits', 'Masked w/out hits', 'Unmasked w/ hits', 'Unmasked w/out hits']
    ax0.pie(sizes, labels=labels, autopct='%1.3f%%')
    ax0.set_title(f'All Categories - Sector {-i}')

    # Create pie charts for masked channels
    masked_sizes = [masked_with_hits / masked_channels, masked_without_hits / masked_channels]
    masked_labels = ['Masked w/ hits', 'Masked w/out hits']
    ax1.pie(masked_sizes, labels=masked_labels, autopct='%1.3f%%')
    ax1.set_title(f'Masked Channels - Sector {-i}')

    # Create pie charts for unmasked channels
    unmasked_sizes = [unmasked_with_hits / umasked_channels, unmasked_without_hits / umasked_channels]
    unmasked_labels = ['Unmasked w/ hits', 'Unmasked w/out hits']
    ax2.pie(unmasked_sizes, labels=unmasked_labels, autopct='%1.3f%%')
    ax2.set_title(f'Unmasked Channels - Sector {-i}')

# Adjust the spacing between subplots
plt.subplots_adjust(wspace=0.4, hspace=0.6)

# Save the figures
fig0.savefig('all_categories_all_sectors.png', dpi=300, bbox_inches='tight')
fig1.savefig('masked_channels_all_sectors.png', dpi=300, bbox_inches='tight')
fig2.savefig('unmasked_channels_all_sectors.png', dpi=300, bbox_inches='tight')
