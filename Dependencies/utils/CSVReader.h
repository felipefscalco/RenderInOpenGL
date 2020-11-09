#include <fstream>
#include <string>
#include <sstream>
#include <vector>

static std::vector<float> read_csv_file(std::string fileName)
{
	std::vector<float> vector;

	// Create an input filestream
	std::ifstream fileStream(fileName);

	// Make sure the file is open
	if (!fileStream.is_open()) throw std::runtime_error("Could not open file");

	std::string line, colName;
	float val;

	if (fileStream.good())
	{
		std::stringstream ss(line);
	}

	// Read data, line by line
	while (std::getline(fileStream, line))
	{
		// Create a stringstream of the current line
		std::stringstream ss(line);

		// Keep track of the current column index
		int columnIndex = 0;

		// Extract each integer
		while (ss >> val) 
		{
			// Add the current integer to the 'colIdx' column's values vector
			vector.push_back(val);

			// If the next token is a comma, ignore it and move on
			if (ss.peek() == ';') ss.ignore();

			// Increment the column index
			columnIndex++;
		}
	}

	fileStream.close();

	return vector;
}