#include <sstream>
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <algorithm>   

#include "Circuit.h"
#include "Simulation.h"
#include "PriorityQueue.h"
 
using namespace std;

int Transition::GlobalId = 0; 

void Transition::Apply()
{
	if (!IsValid())
		throw std::runtime_error("Gate output should not transition to the same value");
	gate->SetOutput(newOutput);
	 
	//cout << " [" << gate->GetName() << " -> " << newOutput << "]";
}

boost::property_tree::ptree Probe::GetJson()
{
	boost::property_tree::ptree children;
	boost::property_tree::ptree pt;
	pt.put_value(time);
	children.push_back(std::make_pair("", pt));

	pt.put_value(gateName);
	children.push_back(std::make_pair("", pt));

	pt.put_value(newValue);
	children.push_back(std::make_pair("", pt));

	return children;
} 

void Simulation::AddTransition(std::string gateName, int outputValue, int outputTime)
{
	Gate* pGate = m_circuit->GetGate(gateName);
	m_inTransitions.emplace_back(Transition{ pGate, outputValue, outputTime });
}

//The 'is' refer to either Json file or non-Json file depends on number of parameters of cmd line.
std::unique_ptr<Simulation> Simulation::FromFile(std::ifstream& is)
{
	auto simulation = std::make_unique<Simulation>();
	//cout << "unique_ptr to simulation: " << simulation << endl;
	auto* circuit = simulation->GetCircuit();
	//cout << "circuit Get() on unique_ptr of m_circuit: " << circuit << endl;
	for (;;)
	{
		boost::char_separator<char> sep(" ");
		std::string line;
		std::getline(is, line);
		if ( line.empty() || (line.size() > 0 && line[0] == '#') ) continue; //YYX
		boost::tokenizer< boost::char_separator<char>> tokens(line, sep);
		std::vector<std::string> command;
		for (const auto& v : tokens)
			command.emplace_back(v);
		if (command.empty())
			continue;
		//else //YYX
		//{
		//	for (auto a : command)
		//		std::cout << a << ' ';
		//	std::cout << std::endl;
		//}

		if (command[0] == "table") // Format: table <name> <output values printed on single line>
		{
			if (command.size() < 4) continue; //YYX
			std::vector<int> outputs;
			for (size_t i = 2; i < command.size(); ++i)
				outputs.emplace_back(std::stoi(command[i]));
			// AddTruthTable got to check existance of this truth table. Done already.
			circuit->AddTruthTable(command[1], outputs);
		}
		else if (command[0] == "type") // Format: type <name> <truth table> <delay>
		{
			if (command.size() != 4)
				throw std::runtime_error("Invalid number of arguments for gate type");
			circuit->AddGateType(command[1], command[2], std::stoi(command[3]));
		}
		else if (command[0] == "gate") // Format: gate <name> <type> [input gate1] [input gate2]
		{
			std::vector<std::string> inputs;
			for (size_t i = 3; i < command.size(); ++i)
				inputs.emplace_back(command[i]);
			circuit->AddGate(command[1], command[2], inputs);
		}
		else if (command[0] == "probe") // Format: probe <gate> 
		{
			if (command.size() != 2)
				throw std::runtime_error("Invalid number of arguments for probe type");
			circuit->AddProbe(command[1]);
		}
		else if (command[0] == "flip") // Format: flip <gate> <value> <time offset>
		{
			if (command.size() != 4)
				throw std::runtime_error("Invalid number of arguments for flip type");
			simulation->AddTransition(command[1], std::stoi(command[2]), std::stoi(command[3]));
		}
		else if (command[0] == "done")
			break;

	}
	return simulation;
}

// Read the 'layout' portion namely the last portion of the test file.
void Simulation::LayoutFromFile(std::ifstream& is)
{
	std::string temp;
	while (std::getline(is, temp))
	{
		if (temp == "layout")
			break;
	}
	
	boost::regex xmlRegex("^<\\?xml.*\\?>\n");
	boost::regex docTypeRegex("^<!DOCTYPE.*?>\n");
	std::ostringstream sstr;
	sstr << is.rdbuf();
	std::string str = sstr.str();
	str = boost::regex_replace(str, xmlRegex, "");
	str = boost::regex_replace(str, docTypeRegex, "");
	m_layout = str;
}
 

int Simulation::Step()
{
	auto start0 = std::chrono::steady_clock::now();
	int stepTime = m_queue.min()->time;
	std::vector<Transition> transitions;
	auto end0 = std::chrono::steady_clock::now();
	duration0 += duration_cast<std::chrono::microseconds>(end0 - start0).count();

	auto start = std::chrono::steady_clock::now();
	while (m_queue.len() > 0 && m_queue.min()->time == stepTime)
	{ 
		auto transition = m_queue.pop();
		if (!transition->IsValid())  // Whether the newOutput is different from the Gata's m_output.
			continue;  
		transition->Apply();   
		   
		string s = "[" + to_string(transition->objectId) + "] "
			+ to_string(transition->time) + " "
			+ transition->gate->GetName() + " "
			+ to_string(transition->newOutput) + "\n";  
		cout << s;
		logFile << s; 
		 
		if (transition->gate->IsProbed())
		{ 
			m_probes.emplace_back(Probe{ transition->time, transition->gate->GetName(), transition->newOutput });
			 
		} 
		transitions.emplace_back(*transition);
		 
	} 
	auto end = std::chrono::steady_clock::now();
	duration += duration_cast<std::chrono::microseconds>(end - start).count();
	// Check the m_outGates of the Gate of every transition which just popped out and executed.
	// for every Gate (eg. Gate_a) in the m_outGates which is so-called 'Connected gate',
	// 1. Find the output value of Gate_a thru it's m_inGates then corresponding TruthTable;
	// 2. Re-Calculate the transition time taking both Gate_a's m_dalay and its GateType's m_dalay;
	// 3. Forming a new Transition of Gate_a and append it to m_queue. 
	auto start2 = std::chrono::steady_clock::now();
	for (const auto transition : transitions)
	{
		for (auto* gate : transition.gate->GetOutGates())
		{
			auto output = gate->GetTransitionOutput();
			auto time = gate->GetTransitionTime(stepTime);
			Transition tr = { gate, output, time };
			m_queue.append(std::make_pair(tr.time, tr.objectId), tr);
			//m_queue.append(Transition(gate, output, time)); 
			string ts = "    [" + to_string(tr.objectId) + "] "
				+ to_string(tr.time) + " "
				+ tr.gate->GetName() + " "
				+ to_string(tr.newOutput) + "\n"; 
			logFile << ts;
		}
	}
	auto end2 = std::chrono::steady_clock::now();
	duration2 += duration_cast<std::chrono::microseconds>(end2 - start2).count();
	return stepTime;
}

void Simulation::Run()
{
	//YYX 
	if (m_inTransitions.size() > 0)
	{
		cout << "No. of Transition input read: " << m_inTransitions.size() << "\n";
	}

	std::sort(m_inTransitions.begin(), m_inTransitions.end());
	for (const auto& t : m_inTransitions)
		m_queue.append(make_pair(t.time, t.objectId), t);  

	logFile.open("logging.txt", std::ios::out);
	auto start = std::chrono::steady_clock::now();
	while (m_queue.len() > 0)
		Step();  
	auto end = std::chrono::steady_clock::now();
	auto dur = duration_cast<std::chrono::microseconds>(end - start).count();
	logFile << "[  while]" << dur << "\n";
	logFile << "[b4while]" << duration0 << "\n";
	logFile << "[--while]" << duration << "\n";
	logFile << "[--for  ]" << duration2 << "\n";
	logFile.close();
	std::sort(m_probes.begin(), m_probes.end()); 
	
}

void Simulation::UndoProbeAllGates()
{
	for (auto* gate : m_undoLog)
	{
		gate->UndoProbe();
	}
	m_undoLog.clear();
}

boost::property_tree::ptree Simulation::GetJson()
{
	boost::property_tree::ptree pt;
	pt.add_child("circuit", m_circuit->GetJson());
	boost::property_tree::ptree probes;
	for (auto& p : m_probes)
		probes.push_back(std::make_pair("",p.GetJson()));
	pt.add_child("trace", probes); 
	pt.add("layout", m_layout);
	return pt;
}

void Simulation::PrintProbes(std::ostream& os)
{
	for (const auto& probe : m_probes)
	{
		if (!m_circuit->GetGate(probe.gateName)->IsProbed())
			continue;
		os << probe.time << " " << probe.gateName << " " << probe.newValue << std::endl;
	}
		
}