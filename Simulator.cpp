// Simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <fstream>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Simulation.h" 

int main(int argc, char** argv)
{
    // Must have 1 parameter at least, not neccessary json file?
    if (argc < 2)
    {
        std::cout << "Simulator.exe <simfile> [json]" << std::endl;
        std::cout << "simulation output is in circuit.jsonp" << std::endl;
        exit(0);
    }
    bool json = (argc >= 3 && "json" == std::string(argv[2]));
    std::ifstream input(argv[1], std::ios::in);

    //std::cout << "Reading inputs from:" << argv[1] << "...\n";
    auto simulation = Simulation::FromFile(input); // return unique_ptr of instance of Simulation.
    
    // Processing only JSON file.
    if (json)
    {
        simulation->LayoutFromFile(input);
        // probe all gates should only be executed when 
        // json output is on 
        simulation->ProbeAllGates();
    }
         
    auto start = std::chrono::steady_clock::now();  
    simulation->Run();
    auto end = std::chrono::steady_clock::now();
    auto duration = duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout <<"[Run] " << duration << "ms\n";

    // Processing only JSON file.
    if (json)
        simulation->UndoProbeAllGates();
     
    // Processing only JSON file, same as if(json)
    if (argc >= 3 && "json" == std::string(argv[2]))
    {
        start = std::chrono::steady_clock::now();
        boost::property_tree::ptree simResult = simulation->GetJson();
        end = std::chrono::steady_clock::now();
        std::cout << "[GetJSON] " << duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

        start = std::chrono::steady_clock::now();
        std::ofstream output("circuit.jsonp", std::ios::out);
        output << "onJsonp(";
        boost::property_tree::write_json(output, simResult);
        output << ");\n";
        end = std::chrono::steady_clock::now();
        std::cout << "[WriteJSON] " << duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";

    } 

    std::cout << "Printing Probes..." << std::endl;
    simulation->PrintProbes(std::cout);
}
