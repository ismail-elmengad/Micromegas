import json
import numpy as np
import matplotlib.pyplot as plt

def checkMMFE8(serialized_data, sector, node):
    # Open the serialized data
    with open(serialized_data) as file:
        data = json.load(file)
    # Load the desired mmfe8
    mmfe8 = data[node]

    # record statistics for the whole vmm
    tot_unmasked_no_hits = 0
    tot_unmasked_hits = 0
    tot_masked_no_hits = 0
    tot_masked_hits = 0

    # initialze a list which will have the four fractions by vmm
    vmm_hit_fractions = []
    vmm_count = 0
    for vmm_key, vmm_value in mmfe8.items():
        unmasked_no_hits = 0
        unmasked_hits = 0
        masked_no_hits = 0
        masked_hits = 0
        for channel in vmm_value:
            if (channel[0] == 0 and channel[sector] == 0):
                unmasked_no_hits += 1
            elif (channel[0] == 0 and channel[sector] == 1):
                unmasked_hits += 1
            elif (channel[0] == 1 and channel[sector] == 0):
                masked_no_hits += 1
            elif (channel[0] == 1 and channel[sector] == 1):
                masked_hits += 1

        # Add to MMFE8 tallies
        tot_unmasked_no_hits += unmasked_no_hits
        tot_unmasked_hits += unmasked_hit_hits
        tot_masked_no_hits += masked_no_hits
        tot_masked_hits += masked_hits

        # The total number of channels in a vmm = 64
        tot = 64
        vmm_hit_fractions.append([masked_hits/tot, masked_no_hits/tot, unmasked_hits/tot, unmasked_no_hits/tot])
        print(node, f" fractions are  {vmm_hit_fractions[vmm_count][0]}:   {vmm_hit_fractions[vmm_count][1]}:   {vmm_hit_fractions[vmm_count][2]}:   {vmm_hit_fractions[vmm_count][3]}\n")
        vmm_count += 1

    return (tot_unmasked_no_hits, tot_unmasked_hits, tot_masked_no_hits, tot_masked_hits)


# Loop through the serialized json file
with open("channel_performance.json") as json_file:
    data = json.load(json_file)

# Create figures with 4 rows and 8 columns of subplots
fig0, axs0 = plt.subplots(nrows=8, ncols=4, figsize=(24, 32))
fig0.suptitle('All Channels - mmg_thresholdValidation1khz_150424_Run473376', fontsize=30)
fig1, axs1 = plt.subplots(nrows=8, ncols=4, figsize=(24, 32))
fig1.suptitle('Masked Channels - mmg_thresholdValidation1khz_150424_Run473376', fontsize=30)
fig2, axs2 = plt.subplots(nrows=8, ncols=4, figsize=(24, 32))
fig2.suptitle('Unmasked Channels - mmg_thresholdValidation1khz_150424_Run473376', fontsize=30)

# Flatten the axs arrays to iterate over each subplot
axs0 = axs0.flatten()
axs1 = axs1.flatten()
axs2 = axs2.flatten()

# Loop through the sectors and create pie charts for each sector
for idx, i in enumerate(list(range(-16, 0)) + list(range(1, 17))):
    unmasked_with_hits = 0
    unmasked_without_hits = 0
    masked_with_hits = 0
    masked_without_hits = 0
    # Loop through MMFE8s
    for mmfe8 in data[str(i)]:
        # Loop through vmms
        for vmm in data[str(i)][mmfe8]:
            # Loop through channels
            for channel in data[str(i)][mmfe8][vmm]:
                if (channel[0] == 0 and channel[1] == 0):
                    unmasked_without_hits += 1
                elif (channel[0] == 0 and channel[1] == 1):
                    unmasked_with_hits += 1
                elif (channel[0] == 1 and channel[1] == 0):
                    masked_without_hits += 1
                elif (channel[0] == 1 and channel[1] == 1):
                    print(f'{i}\n')
                    masked_with_hits += 1

    # Get various fractions of masking/hit statistics
    total_channels = unmasked_with_hits + unmasked_without_hits + masked_with_hits + masked_without_hits
    masked_channels = masked_with_hits + masked_without_hits
    umasked_channels = unmasked_with_hits + unmasked_without_hits

    # Create pie charts for all four categories
    sizes = [masked_with_hits / total_channels, masked_without_hits / total_channels,
             unmasked_with_hits / total_channels, unmasked_without_hits / total_channels]
    labels = ['Masked w/ hits', 'Masked w/out hits', 'Unmasked w/ hits', 'Unmasked w/out hits']
    axs0[idx].pie(sizes, autopct='%1.3f%%')
    axs0[idx].set_title(f'Sector {i}')

    # Create pie charts for masked channels
    masked_sizes = [masked_with_hits / masked_channels, masked_without_hits / masked_channels]
    masked_labels = ['Masked w/ hits', 'Masked w/out hits']
    axs1[idx].pie(masked_sizes, autopct='%1.3f%%')
    axs1[idx].set_title(f'Sector {i}')

    # Create pie charts for unmasked channels
    unmasked_sizes = [unmasked_with_hits / umasked_channels, unmasked_without_hits / umasked_channels]
    unmasked_labels = ['Unmasked w/ hits', 'Unmasked w/out hits']
    axs2[idx].pie(unmasked_sizes, autopct='%1.3f%%')
    axs2[idx].set_title(f'Sector {i}')

# Add legends to the dummy plots
fig0.legend(labels, loc='lower center', ncol=4, bbox_to_anchor=(0.5, -0.1), fontsize=25)
fig1.legend(masked_labels, loc='lower center', ncol=2, bbox_to_anchor=(0.5, -0.1), fontsize=50)
fig2.legend(unmasked_labels, loc='lower center', ncol=2, bbox_to_anchor=(0.5, -0.1), fontsize=50)

# Alternatively, use subplots_adjust
fig0.subplots_adjust(wspace=0.2, hspace=0.1)
fig1.subplots_adjust(wspace=0.2, hspace=0.2)
fig2.subplots_adjust(wspace=0.2, hspace=0.2)

# Save the figures
fig0.savefig('all_categories_all_sectors.png', dpi=300, bbox_inches='tight')
fig1.savefig('masked_channels_all_sectors.png', dpi=300, bbox_inches='tight')
fig2.savefig('unmasked_channels_all_sectors.png', dpi=300, bbox_inches='tight')
