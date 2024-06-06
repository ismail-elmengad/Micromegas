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



void serialize_data(std::string infile) {

    // Open the JSON file
    std::ifstream jsonfile("/afs/cern.ch/user/i/ielmenga/public/Micromegas/C01.json");
    
    // Check if the file was opened successfully
    if (!jsonfile.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }

    // Parse the JSON data
    json data;
    jsonfile >> data;
    
    // Iterate through the data keys and make another map for every top level key beginning with MMFE8
    std::map<std::string, std::map<std::string, std::vector<std::vector<int>>>> hitMap;

    for (auto& [key, value] : data.items()) {
        // Add all the MMFE8's to the top level of the dictionary
        if (key.rfind("MMFE8_", 0) == 0) {
            hitMap.insert({key, std::map<std::string, std::vector<std::vector<int>>>()});
            // Add a vmm dictionary to the next level within each mmfe8
            for (int i=0; i<8; i++) {
                // Create the key value for each vmm
                std::string vmm_label = "vmm" + std::to_string(i);
                // Assign to that key a vector of vector of ints. The first int in the final vector is the
                // masking status of the channel. the 2nd hit is whether sector -1 has hits, the 3rd for sector -2 and so on
                hitMap[key][vmm_label] = std::vector<std::vector<int>>(64, std::vector<int>(17, 0));

                
                // Now loop through all the vmms and set the masking status on the first vector element
                for (int j=0; j<64; j++) {
                    hitMap[key][vmm_label].at(j).at(0) = data[key][vmm_label]["channel_sm"].at(j);
                }
            }
        }
    }

    // Open TFiles
    TFile *file = TFile::Open(infile.c_str(), "READ");
    if (!file || file->IsZombie()) {
        // Handle error
        std::cout << "Could not open file" << std::endl;
        return;
    }

    // INitialize tree and set branches
    TTree *tree = (TTree*)file->Get("nsw");

    tree->SetBranchStatus("*", false);
    tree->SetBranchStatus("strip", true);
    tree->SetBranchStatus("vmmid", true);
    tree->SetBranchStatus("layer", true);
    tree->SetBranchStatus("nhits", true);
    tree->SetBranchStatus("radius", true);
    tree->SetBranchStatus("channel", true);
    tree->SetBranchStatus("sector", true);

    std::vector<int> *strips = nullptr;
    std::vector<unsigned int> *vmmids = nullptr;
    std::vector<unsigned int> *layers = nullptr;
    std::vector<unsigned int> *hits = nullptr;
    std::vector<unsigned int> *radii = nullptr;
    std::vector<unsigned int> *channels = nullptr;
    std::vector<int> *sectors = nullptr;

    tree->SetBranchAddress("strip", &strips);
    tree->SetBranchAddress("vmmid", &vmmids);
    tree->SetBranchAddress("layer", &layers);
    tree->SetBranchAddress("nhits", &hits);
    tree->SetBranchAddress("radius", &radii);
    tree->SetBranchAddress("channel", &channels);
    tree->SetBranchAddress("sector", &sectors);
    

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

            // If there is a hit at the channel, set the hit status for the appropriate sector to 1
            hitMap[node][vmm_label].at(channels->at(n)).at(-1 * sectors->at(n)) = 1;
        }
    }

    // Now export the dictionary
    nlohmann::json jsonObject = nlohmann::json(hitMap);
    std::ofstream hitFile("channel_performance.json");
    hitFile << jsonObject;
    file->Close();
}


int main() {
    serialize_data("/data1/mmg_thresholdValidation1khz_150424/data_test.00473379.calibration_1khz.daq.RAW._lb0000._SFO-1._0005.simple.root");
    return 0;
}