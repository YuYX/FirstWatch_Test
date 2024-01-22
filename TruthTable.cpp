#include <stdexcept>
#include "TruthTable.h"

// Bear in mind the size of m_table can be either 2 or 4
//   
int TruthTable::GetOutput(std::vector<int> inputs) const
{
	if (m_inputCount == 1)
		return m_table[inputs[0]];
	return m_table[inputs[0] * 2 + inputs[1]];
}

