import json
import numpy as np
import matplotlib.pyplot as plt


# Loop through the serialized json file
with open("channel_performance.json") as json_file:
    data = json.load(json_file)

fig, axs = plt.subplots(nrows=4, ncols=4, figsize=(16, 12))

axs = axs.flatten()



# Loop through the sectors
for i, ax in enumerate(axs, start=1):
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
    channels_with_hits = unmasked_with_hits + masked_with_hits
    channels_without_hits = total_channels - channels_with_hits
    labels = 'Masked w/ hits', 'Masked w/out hits', 'Unmasked w/ hits', 'Unmasked w/out hits'

    # Turn this sector data into a piechart
    sizes = [x / total_channels for x in [masked_with_hits, masked_without_hits, unmasked_with_hits, unmasked_without_hits]]
    fig, ax = plt.subplots()
    ax.pie(sizes, labels=labels, autopct='%1.3f%%')
    ax.set_title(f'Channel Performance for Sector {i}')
    fig.savefig(f'performance_sector_{i}.png')

plt.subplots_adjust(wspace(0.4, hspace=0.6))

plt.savefig('channel_performance_all_sectors.png', dpi=300, bbox_inches='tight')
