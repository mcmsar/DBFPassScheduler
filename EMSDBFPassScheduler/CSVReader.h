//#include <iterator>
//#include <iostream>
#include <fstream>
//#include <sstream>
//#include <vector>
//#include <string>

class CSVRow
{
    public:
        std::string const& operator[](std::size_t index) const
        {
            return m_data[index];
        }
        std::size_t size() const
        {
            return m_data.size();
        }
        void readNextRow(std::istream& str)
        {
            std::string         line;
            std::getline(str, line);

            std::stringstream   lineStream(line);
            std::string         cell;

            m_data.clear();
            while(std::getline(lineStream, cell, ','))
            {
                m_data.push_back(cell);
            }
            // This checks for a trailing comma with no data after it.
            if (!lineStream && cell.empty())
            {
                // If there was a trailing comma then add an empty element.
                m_data.push_back("");
            }
        }
    private:
        std::vector<std::string>    m_data;
};

std::istream& operator>>(std::istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
} 

template<typename T>
std::string ToString(const T& v)
{
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

template<typename T>
T FromString(const std::string& str)
{
    std::istringstream ss(str);
    T ret;
    ss >> ret;
    return ret;
}

std::string ConvertInt64toStr(INT64 &bcnId)
{			
	std::stringstream ss;
	ss << std::hex << bcnId;
	return ss.str();
}


std::string tokenize(const std::string& s, int nChars) {
   if (!s.size()) {
     return "";
   }
   std::stringstream ss;
   ss << s[0];
   int j = 1;
   for (int i = 1; i < s.size(); i++) {
	   if(i == nChars*j)
	   {
			ss << ' ' << s[i];
			j++;
	   }
	   else
	   {
			ss << s[i];
	   }
   }
   return ss.str();
}