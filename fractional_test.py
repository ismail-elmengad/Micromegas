import ROOT
import json
import numpy as np
import matplotlib.pyplot as plt

unmasked_with_hits = 0
unmasked_without_hits = 0
masked_with_hits = 0
masked_without_hits = 0

# Loop through the serialized json file
with open("channel_performance.json") as json_file:
    data = json.load(json_file)
    # Loop through MMFE8s
    for key in data:
        # Loop through vmms
        for vmm in data[key]:
            # Loop through channels
            for channel in data[key][vmm]:
                if (channel[0] == 0 and channel[1] == 0):
                    unmasked_without_hits += 1
                elif (channel[0] == 0 and channel[1] == 1):
                    unmasked_with_hits += 1
                elif (channel[0] == 1 and channel[1] == 0):
                    masked_without_hits += 1
                elif (channel[0] == 1 and channel[1] == 1):
                    masked_with_hits += 1

# Get various fractions of masking/hit statistics
total_channels = unmasked_with_hits + unmasked_without_hits + masked_with_hits + masked_without_hits
masked_channels = masked_with_hits + masked_without_hits
umasked_channels = total_channels - masked_channels
channels_with_hits = unmasked_with_hits + masked_with_hits
channels_without_hits = total_channels - channels_with_hits

# Plot this data as a pychart
labels = 'Masked w/ hits', 'Masked w/out hits', 'Unmasked w/ hits', 'Unmasked w/out hits'
sizes = [x / total_channels for x in [masked_with_hits, masked_without_hits, unmasked_with_hits, unmasked_without_hits]]
fig, ax = plt.subplots()
ax.pie(sizes, labels=labels, autopct='%1.3f%%')
fig.savefig('performance.png')
