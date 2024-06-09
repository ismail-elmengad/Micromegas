#!/bin/bash

# Step 1: Clone the repository
git clone https://gitlab.cern.ch/atlas-muon-nsw-daq/infrastructure/official-json-configs.git

# Step 2: Navigate to the directory
cd official-json-configs/mmg/re

# Step 3: Copy the JSON files to the desired location
DESTINATION="config_files/"  # Change this to your desired path
mkdir -p $DESTINATION
for i in $(seq -f "%02g" 1 16)
do
    cp official-json-configs/mmg/readout/A$i.json $DESTINATION
    cp official-json-configs/mmg/readout/C$i.json $DESTINATION
    echo "copied A$i.json and C$i.json to $DESTINATION"
done

# Step 5: Move the config files folder to wd and remove the cloned folder
cp -r $DESTINATION

pwd
rm -rf official-json-configs/

echo "Files copied to $DESTINATION and the repository folder has been removed"
