#include <stdexcept>
#include "Gate.h"


namespace
{
	void RaiseError(const char* c) { throw std::runtime_error(c); }
}
 
// 
void Gate::ConnectInput(int i, Gate* target)
{ 
	if (m_inGates.find(i) != m_inGates.end())
		throw std::runtime_error("input terminal already connected");
	m_inGates.insert({ i, target });
	target->AddOutput(this);
}

// Create and add target gate into m_outGates (vector).
void Gate::AddOutput(Gate* target)
{
	m_outGates.emplace_back(target);
}
 
void Gate::Probe() noexcept
{
	if (m_probed)
		RaiseError("Gate already probed");
		
	m_probed = true;
}

boost::property_tree::ptree Gate::GetJson()
{
	// return type of boost::property_tree::ptree.
	/*
	 {
		"id":		m_name,		// Gate's name
		"table":	m_name,		// TruthTable's name
		"type":		m_name,		// GateType's name
		"probed":	m_probed,	// true or false
		"inputs":
					[
						""	:	m_name,	//Gate's name from inGates
						""	:   m_name,
						...
					]
		"outputs":
					[
						""	:	m_name,	//Gate's name from outGates
						""	:   m_name,
						...
					]
	 }
	*/
	boost::property_tree::ptree pt;
	pt.add("id", m_name);
	pt.add("table", m_type->GetTruthTableName());
	pt.add("type", m_type->GetType());
	pt.add("probed", m_probed);

	boost::property_tree::ptree inputs, outputs;
	for (auto& [k,v] : m_inGates)
	{
		boost::property_tree::ptree pt;
		pt.put_value(v->GetName());
		inputs.push_back(std::make_pair("", pt));   
	}
	pt.add_child("inputs", inputs);
	 
	for (auto& v : m_outGates)
	{
		boost::property_tree::ptree pt;
		pt.put_value(v->GetName());
		outputs.push_back(std::make_pair("", pt));
	}
	pt.add_child("outputs", outputs);
	return pt;
}

int Gate::GetTransitionOutput() const
{
	std::vector<int> inputs;
	for (const auto& [k, v] : m_inGates)
		inputs.emplace_back(v->GetOutput());	// m_output of Gates in m_inGates
	return m_type->GetOutput(inputs);			// => GateType => TruthTable.GetOutput(inputs)
}

int Gate::GetTransitionTime(int time) const
{
	return time + m_type->GetDelay();
}

void Gate::UndoProbe()
{
	m_probed = false;
}
