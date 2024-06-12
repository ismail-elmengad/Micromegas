#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <set>
#include <map>
#include <tuple>
#include <string>
#include <vector>
#include <algorithm>
#include <TFile.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <TROOT.h>
#include <TChain.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <THStack.h>
#include <TVector.h>
#include <TVectorT.h>
#include <TVectorF.h>
#include <TMultiGraph.h>
#include <TCanvas.h>
#include <TBranch.h>
#include <TMath.h>
#include <TF1.h>
#include <TPaveText.h>

using json = nlohmann::json;

// A function to convert from a node label to an integer (vmmid)
std::pair<unsigned int, unsigned int> nodeToInts(const std::string& mmfe8) {
    unsigned int o_layer = mmfe8[7] - '0';
    unsigned int o_pcb = mmfe8[9] - '0';
    std::string o_quad = mmfe8.substr(11, 2);
    char o_side = mmfe8.back();
    unsigned int radius = (o_pcb - 1) * 2;

    if (o_quad == "HO") { // layer >= 4
        o_layer = 8 - o_layer;
    } else {
        o_layer -= 1;
    }     

    if (!((o_side == 'R' && o_layer % 2) || (o_side == 'L' && !(o_layer % 2)))) {
        radius += 1;
    }

    return std::make_pair(o_layer, radius);
}

// A function to convert from an integer label (vmmid) to Node
std::string intsToNode(unsigned int layer, unsigned int radius) {
    unsigned int o_layer = (layer < 4) ? layer + 1 : 8 - layer;
    unsigned int o_pcb = radius / 2 + 1;
    std::string o_quad = (layer >= 4) ? "HO" : "IP";
    char o_side = ((radius + layer) % 2) ? 'R' : 'L';
    std::string o_nodeName = "MMFE8_L" + std::to_string(o_layer) + "P" + std::to_string(o_pcb) + "_" + o_quad + o_side;
    return o_nodeName;
}

// Function to create a TH1D histogram with a given name
TH1D createHistogram(const std::string& name) {
    int nBins = 8 * 64;     // Each MMFE8 has 8 vmms with 64 channels each
    TH1D hist(name.c_str(), name.c_str(), nBins, 0, nBins);
    return hist;
}


// Characterizes the effective of masking
void characterize(std::string infile, std::string outfile) {


    // Open the JSON file
    std::ifstream jsonfile("C01.json");
    
    // Check if the file was opened successfully
    if (!jsonfile.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }
    
    // Parse the JSON data
    json data;
    jsonfile >> data;
    
    // Iterate through the data keys and make a hitsogram every time a top level key begins with MMFE8. Then add it to a map
    std::map<std::string, TH1D> mmfMap;

    for (auto& [key, value] : data.items()) {
        if (key.rfind("MMFE8_", 0) == 0) {
            TH1D hist = createHistogram(key);
            mmfMap[key] = hist;
            std::cout << "created map for "<< key << std::endl;
        }
    }

    // Open TFiles
    TFile *file = TFile::Open(infile.c_str(), "READ");
    if (!file || file->IsZombie()) {
        // Handle error
        std::cout << "Could not open file" << std::endl;
        return;
    }

    // Initialize tree and set branches
    TTree *tree = (TTree*)file->Get("nsw");
    
    tree->SetBranchStatus("*", false);
    tree->SetBranchStatus("strip", true);
    tree->SetBranchStatus("vmmid", true);
    tree->SetBranchStatus("layer", true);
    tree->SetBranchStatus("nhits", true);
    tree->SetBranchStatus("radius", true);
    tree->SetBranchStatus("channel", true);

    std::vector<int> *strips = nullptr;
    std::vector<unsigned int> *vmmids = nullptr;
    std::vector<unsigned int> *layers = nullptr;
    std::vector<unsigned int> *hits = nullptr;
    std::vector<unsigned int> *radii = nullptr;
    std::vector<unsigned int> *channels = nullptr;

    tree->SetBranchAddress("strip", &strips);
    tree->SetBranchAddress("vmmid", &vmmids);
    tree->SetBranchAddress("layer", &layers);
    tree->SetBranchAddress("nhits", &hits);
    tree->SetBranchAddress("radius", &radii);
    tree->SetBranchAddress("channel", &channels);
    

    // Loop through all tree entries
    int nentries = tree->GetEntries();
    for (int i=0; i<nentries; i++) {
        // Progress bar
        if (i % 100 == 0) { std::cout << i << "/" << nentries << " entries processed\n"; }

        tree->GetEntry(i);
        // Loop through events in the entry
        for (unsigned int n=0; n<strips->size(); n++) {
            
            //Produce the node label from the layer and radius
            std::string node = intsToNode(layers->at(n), radii->at(n));

            // Find the node in the JSON data
            auto mmfe8 = data.at(node);
            // Get the mmf dictionary
            std::string vmm_label = "vmm" + std::to_string(vmmids->at(n));
            auto vmm = mmfe8.at(vmm_label);
            // Get the channel maskings
            auto channel_maskings = vmm.at("channel_sm");
            int channel_mask = channel_maskings[channels->at(n)];
            
            // Define a new 'mmf_channel'. There are 4096 for a specific node (8vmms * 64channels)
            int mmf_channel = vmmids->at(n) * 64 + channels->at(n);

            // Only write to file the hits when the channel is masked
            if (channel_mask == 1) {
                mmfMap.at(node).Fill(mmf_channel);
            }
        }
    }

    // Open output file and write the histograms to it
    TFile *output = new TFile(outfile.c_str(), "RECREATE");
    for (auto& [key, value] : mmfMap) {
        value.Write();
    }
    output->Close();
}



void serialize_data(std::string indir, std::string run) {

    // Array to hold the file names
    std::string filenames[32] = {
        "C16.json", "C15.json", "C14.json", "C13.json", "C12.json", "C11.json", "C10.json", "C09.json",
        "C08.json", "C07.json", "C06.json", "C05.json", "C04.json", "C03.json", "C02.json", "C01.json",
        "A01.json", "A02.json", "A03.json", "A04.json", "A05.json", "A06.json", "A07.json", "A08.json",
        "A09.json", "A10.json", "A11.json", "A12.json", "A13.json", "A14.json", "A15.json", "A16.json"
    };

    // Array to hold the JSON data objects
    json data[32];

    // Loop through each file and load the JSON data
    for (int i = 0; i < 32; ++i) {
        // Open the JSON file
        std::ifstream jsonfile("config_files/" + filenames[i]);
        std::cout << "data[" << i << "] is from " << filenames[i] << std::endl;

        // Check if the file was opened successfully
        if (!jsonfile.is_open()) {
            std::cerr << "Failed to open file: " << filenames[i] << std::endl;
            return;
        }

        // Parse the JSON data
        jsonfile >> data[i];

        // Close the file
        jsonfile.close();
    }
    
    // A map from sector -> MMFE8, MMFE8->vmm, vmm->Channel, channel has 2 elements, masking status & hits present
    std::map<std::string, std::map<std::string, std::map<std::string, std::vector<std::vector<int>>>>> hitMap;

    // Loop through sectors
    for (int j=-16; j<0; j++) {
        std::cout << "sector " << j << " from " << filenames[j+16] << std::endl;
        // Iterate through the data keys and make another map for every key beginning with MMFE8 in the sector
        for (auto& [key, value] : data[j+16].items()) {
            if (key.rfind("MMFE8_", 0) == 0) {
                hitMap[std::to_string(j)].insert({key, std::map<std::string, std::vector<std::vector<int>>>()});
                // Add a vmm dictionary to the next level within each mmfe8
                for (int i=0; i<8; i++) {
                    // Create the key value for each vmm
                    std::string vmm_label = "vmm" + std::to_string(i);
                    // Assign to that key a vector of 2 ints. The first int in the final vector is the
                    // masking status of the channel. the 2nd hit is whether the channel has hits
                    hitMap[std::to_string(j)][key][vmm_label] = std::vector<std::vector<int>>(64, std::vector<int>(2, 0));
                    // Now loop through all the vmms and set the masking status on the first vector element
                    for (int k=0; k<64; k++) {
                        hitMap[std::to_string(j)][key][vmm_label].at(k).at(0) = data[j+16][key][vmm_label]["channel_sm"].at(k);
                    }
                }
            }
        }
    }

    // Skip j==0
    for (int j=1; j<17; j++) {
        std::cout << "sector " << j << " from " << filenames[j+15]<< std::endl;
        // Iterate through the data keys and make another map for every key beginning with MMFE8 in the sector
        for (auto& [key, value] : data[j+15].items()) {
            if (key.rfind("MMFE8_", 0) == 0) {
                hitMap[std::to_string(j)].insert({key, std::map<std::string, std::vector<std::vector<int>>>()});
                // Add a vmm dictionary to the next level within each mmfe8
                for (int i=0; i<8; i++) {
                    // Create the key value for each vmm
                    std::string vmm_label = "vmm" + std::to_string(i);
                    // Assign to that key a vector of 2 ints. The first int in the final vector is the
                    // masking status of the channel. the 2nd hit is whether the channel has hits
                    hitMap[std::to_string(j)][key][vmm_label] = std::vector<std::vector<int>>(64, std::vector<int>(2, 0));

                    // Now loop through all the vmms and set the masking status on the first vector element
                    for (int k=0; k<64; k++) {
                        // Check if the key "channel_sm" exists in the JSON object
                        if (data[j+15][key][vmm_label].contains("channel_sm")) {
                            // Check if the value at index k is not null
                            if (!data[j+15][key][vmm_label]["channel_sm"].at(k).is_null()) {
                                hitMap[std::to_string(j)][key][vmm_label].at(k).at(0) = data[j+15][key][vmm_label]["channel_sm"].at(k);
                            } else {
                                std::cout << "      Index " << k << " is null" << std::endl;
                                hitMap[std::to_string(j)][key][vmm_label].at(k).at(0) = 2; // Assign a default value of 2 when there is no masking set
                            }
                        } else {
                            // If the key "channel_sm" doesn't exist, handle it appropriately
                            // (e.g., assign a default value to the entire vector)
                            hitMap[std::to_string(j)][key][vmm_label].at(k) = std::vector<int>{2, 0}; // Assign a default vector {2, 0}
                        }
                    }
                }
            }
        }
    }

    // Initalize the directory that was input to this function and retrieve 
    // the file within
    TSystemDirectory dir(indir.c_str(), indir.c_str());
    TList *files = dir.GetListOfFiles();

    // Intiialize the TChain
    TChain chain("nsw");

    // Iterate through the files and add them to the chain if they are valid root files
    if (files) {
        TSystemFile *file;
        TString fname;
        TIter next(files);
        while ((file = (TSystemFile*)next())) {
            fname = file->GetName();
            std::cout << "Opened: " << fname << std::endl;
            if (!file->IsDirectory() && fname.EndsWith(".root")) {
                if (fname.Contains(run.c_str())) {
                    const char* fnameData = fname.Data();
                    if (fnameData != nullptr) {
                        std::string result = indir + std::string(fnameData);
                        std::cout << "File path: " << result << std::endl;
                        chain.Add(result.c_str());
                        std::cout << "Added to chain" << std::endl;
                    } else {
                        std::cerr << "Error: fname.Data() returned a null pointer" << std::endl;
                    }
                }
            }
        }
    }

    chain.SetBranchStatus("*", false);
    chain.SetBranchStatus("strip", true);
    chain.SetBranchStatus("vmmid", true);
    chain.SetBranchStatus("layer", true);
    chain.SetBranchStatus("nhits", true);
    chain.SetBranchStatus("radius", true);
    chain.SetBranchStatus("channel", true);
    chain.SetBranchStatus("sector", true);

    std::vector<int> *strips = nullptr;
    std::vector<unsigned int> *vmmids = nullptr;
    std::vector<unsigned int> *layers = nullptr;
    std::vector<unsigned int> *hits = nullptr;
    std::vector<unsigned int> *radii = nullptr;
    std::vector<unsigned int> *channels = nullptr;
    std::vector<int> *sectors = nullptr;

    chain.SetBranchAddress("strip", &strips);
    chain.SetBranchAddress("vmmid", &vmmids);
    chain.SetBranchAddress("layer", &layers);
    chain.SetBranchAddress("nhits", &hits);
    chain.SetBranchAddress("radius", &radii);
    chain.SetBranchAddress("channel", &channels);
    chain.SetBranchAddress("sector", &sectors);

    // Loop through all tree entries
    int nentries = chain.GetEntries();
    for (int i=0; i<nentries; i++) {

        // Progress bar
        if (i % 1000 == 0) { std::cout << i << "/" << nentries << " entries processed\n"; }
        chain.GetEntry(i);
        // Loop through events in the entry
        for (unsigned int n=0; n<strips->size(); n++) {
            //Produce the node label from the layer and radius
            std::string node = intsToNode(layers->at(n), radii->at(n));
            // Get the mmf dictionary
            std::string vmm_label = "vmm" + std::to_string(vmmids->at(n));
            // If there is a hit at the channel, set the hit status for the appropriate sector to 1
            // hitmap[sector][MMFE8][vmm][channel][hits present] = 1
            hitMap[std::to_string(sectors->at(n))][node][vmm_label].at(channels->at(n)).at(1) = 1;
        }
    }

    // Now export the dictionary
    nlohmann::json jsonObject = nlohmann::json(hitMap);
    std::ofstream hitFile("channel_performance.json");
    hitFile << jsonObject;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_directory> <run_number>" << std::endl;
    }
    serialize_data(argv[1], argv[2]);
}