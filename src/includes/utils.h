#pragma once

#include <QString>

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

namespace passcave {
	void setStdinEcho(bool enable = true);
	std::string getPasswd(std::string prompt = "Enter the password: ");

	/**
	 * This function returns a map containing option names and values.
	 *
	 * This function supports short options (starting with one dash "-"), and long options (starting with two dashes).
	 * In both cases, the key corresponding to these options in returned map have their "-" stripped.
	 * The short options can be chained (using -abc).
	 * If you specify an option starting with three or more dashes, then it is interpreted as
	 * a positional argument.
	 * Also you cannot specify an option name containing the equal sign.
	 *
	 * Option value follow their option name in the command line.
	 * They are separated by a space or by an equal sign.
	 * Note that, in this last case, there shouldn't be any space between option name, the equal sign and the option value.
	 * Otherwise, the equal sign is interpreted as the value of the last option name or as a positional argument.
	 * If an option is repeated with different values, then the returned map will associate the latest value with that option.
	 *
	 * If an option has no value, then an empty string is associated with its name.
	 * If a value has no name associated with it (i.e. positional argument), then its key is built using a dash followed by a sequential number.
	 * Use the function {@link isPositionalArgument} to test for such cases.
	 * @param argc
	 * @param argv
	 * @return an unordered map where keys represent option names, and values represent option values.
	 */
	std::unordered_map<std::string, std::string> getCmdOptions(int argc, char** argv);

	bool isPositionalArgument(std::string optKey);

	std::string convertHex(void const* data, int dataSize);

	std::string currentTime();
	std::string currentDate();
	std::string currentDateTime();

	std::string timeToStr(struct tm const& t);
	std::string dateToStr(struct tm const& t);
	std::string dateTimeToStr(struct tm const& t);

	inline char simple_lowercase(char const& c);
	std::string simple_lowercase(std::string s);

	std::string simple_trim(std::string const& s);

	std::string simple_clearURIPrefix(std::string const& s);

	int simple_compareStrings(std::string const& s1, std::string const& s2);
	int simple_compareStringDates(std::string const& s1, std::string const& s2);
	int simple_compareStringNumbers(std::string const& s1, std::string const& s2);
	int simple_compareStringURIs(std::string const& s1, std::string const& s2);

	void secureErase(void* start, int len);
	void secureErase(std::string& s);
	void secureErase(std::wstring& s);
	void secureErase(std::vector<char>& v);
	void secureErase(QString& s);

	bool fileExists(std::string f);

	std::vector<char> bzipCompress(void const* in, int inSize);
	std::vector<char> bzipUncompress(void const* in, int inSize);
}

