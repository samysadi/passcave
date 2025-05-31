#include "utils.h"

#include <QByteArray>

#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace passcave;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
void passcave::setStdinEcho(bool enable) {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hStdin, &mode);

	if( !enable )
		mode &= ~ENABLE_ECHO_INPUT;
	else
		mode |= ENABLE_ECHO_INPUT;

	SetConsoleMode(hStdin, mode);
}
#else
#include <termios.h>
#include <unistd.h>
void passcave::setStdinEcho(bool enable) {
	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);
	if( !enable )
		tty.c_lflag &= ~ECHO;
	else
		tty.c_lflag |= ECHO;

	(void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}
#endif

std::string passcave::getPasswd(std::string prompt) {
	std::string passwd;
	std::cout << prompt;
	setStdinEcho(false);
	getline(std::cin, passwd);
	setStdinEcho(true);
	std::cout << std::endl;
	return passwd;
}

inline std::string gco_makePositionalArgumentKey(int& pCnt) {
	return std::string("-" + std::to_string(pCnt++));
}

std::unordered_map<std::string, std::string> passcave::getCmdOptions(int argc, char** argv) {
	std::unordered_map<std::string, std::string> r;

	int i = 1; //skip command name
	int pCnt = 0;
	std::string optionName;
	while (i < argc) {
		bool hasTokenValue = false;
		size_t eqPos;
		std::string tokenValue;
		std::string token(*(argv + i++));

		if (token.empty())
			continue;
		if (token.length() == 1)
			goto ADD_TOKEN_AS_VALUE; //only one char: that's a value
		if (token[0] != '-')
			goto ADD_TOKEN_AS_VALUE; //not starting with a dash: that's a value

		eqPos = token.find('=');
		if (eqPos != std::string::npos) {
			hasTokenValue = true;
			tokenValue = token.substr(eqPos + 1);
			token = token.substr(0, eqPos);
			if (token.length() == 1) {
				if (!optionName.empty()) {
					r[optionName] = "";
					optionName = "";
				}
				r[""] = tokenValue;
				continue;
			}
		}

		if (token[1] != '-') {
			//short option(s)
			for (int j = 1; j < token.length(); j++) {
				if (token[j] == '-')
					continue;
				if (!optionName.empty())
					r[optionName] = "";
				optionName = token[j];
			}
			if (!optionName.empty() && hasTokenValue) {
				r[optionName] = tokenValue;
				optionName = "";
			}
		} else {
			//long option
			if (token.length() == 2)
				goto ADD_TOKEN_AS_VALUE; //only two dashes: that's a value
			if (token[2] == '-')
				goto ADD_TOKEN_AS_VALUE; //starting with three dashes: that's a value
			if (!optionName.empty())
				r[optionName] = "";
			optionName = token.substr(2);
			if (hasTokenValue) {
				r[optionName] = tokenValue;
				optionName = "";
			}
		}
		continue;

		ADD_TOKEN_AS_VALUE:
		if (optionName.empty())
			r[std::string("-" + std::to_string(pCnt++))] = token;
		else {
			r[optionName] = token;
			optionName = "";
		}
	}
	if (!optionName.empty())
		r[optionName] = "";
	return r;
}

bool passcave::isPositionalArgument(std::string optKey) {
	return optKey.length() != 0 && optKey[0] == '-';
}

std::string passcave::convertHex(void const* data, int dataSize) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0');

	for (int i = 0; i < dataSize; i++)
		ss << std::setw(2) << static_cast<unsigned>(*(static_cast<unsigned char const*>(data) + i));

	return ss.str();
}

std::string passcave::currentTime() {
	time_t t = std::time(0);
	struct tm* now = localtime(&t);
	return timeToStr(*now);
}

std::string passcave::currentDate() {
	time_t t = std::time(0);
	struct tm* now = localtime(&t);
	return dateToStr(*now);
}

std::string passcave::currentDateTime() {
	time_t t = std::time(0);
	struct tm* now = localtime(&t);
	return dateTimeToStr(*now);
}

std::string passcave::timeToStr(struct tm const& t) {
	return std::to_string(t.tm_hour) + ":" + std::to_string(t.tm_min) + ":" +  std::to_string(t.tm_sec);
}

std::string passcave::dateToStr(struct tm const& t) {
	return std::to_string(t.tm_year + 1900) + "-" + std::to_string(t.tm_mon + 1) + "-" +  std::to_string(t.tm_mday);
}

std::string passcave::dateTimeToStr(struct tm const& t) {
	return timeToStr(t) + "T" + dateToStr(t);
}

inline char passcave::simple_lowercase(char const& c) {
	return ::tolower(c);
}

std::string passcave::simple_lowercase(std::string s) {
	 for (auto& c: s)
		 c = simple_lowercase(c);
	 return s;
}

std::string passcave::simple_trim(std::string const& s) {
	int i = 0;
	while (i < s.length() && ::isspace(s[i]))
		i++;

	int j = s.length()-1;
	while (j >= 0 && ::isspace(s[j]))
		j--;

	int d = j - i + 1;
	if (d <= 0)
		return std::string();

	return s.substr(i, d);
}

inline bool isNullChar(unsigned char const& c) {
	return (c <= 32);
}

int passcave::simple_compareStrings(std::string const& s1, std::string const& s2) {
	int i1 = 0;
	int i2 = 0;
	while (1) {
		char c1 = 0;
		while (i1 < s1.length()) {
			c1 = simple_lowercase(s1[i1]);
			i1++;
			if (!isNullChar(c1))
				break;
		}

		char c2 = 0;
		while (i2 < s2.length()) {
			c2 = simple_lowercase(s2[i2]);
			i2++;
			if (!isNullChar(c2))
				break;
		}

		if (c1 == 0) {
			if (c2 == 0) {
				if (s1.length() < s2.length())
					return -1;
				else if (s1.length() > s2.length())
					return 1;
				else
					return 0;
			} else
				return -1;
		} else {
			if (c2 == 0)
				return 1;
			else {
				if (c1 < c2)
					return -1;
				else if (c1 > c2)
					return 1;
			}
		}
	}
}

int passcave::simple_compareStringNumbers(std::string const& s1, std::string const& s2) {
	long long a1 = std::strtoll(s1.c_str(), NULL, 0);
	long long a2 = std::strtoll(s2.c_str(), NULL, 0);

	long long d = a1 - a2;
	if (d < 0)
		return -1;
	else if (d == 0)
		return 0;
	else
		return 1;
}

int passcave::simple_compareStringDates(std::string const& s1, std::string const& s2) {
	char const* p1 = s1.c_str();
	char const* p2 = s2.c_str();

	while (*p1 != '\0' && *p2 != '\0') {
		char* pp;
		long long a1 = strtoll(p1, &pp, 10); p1 = pp;
		long long a2 = strtoll(p2, &pp, 10); p2 = pp;

		if (a1 == a2)
			continue;
		if (a1 < a2)
			return -1;
		else
			return 1;
	}

	return 0;
}

std::string passcave::simple_clearURIPrefix(std::string const& s) {
	std::string r = simple_trim(s);

	std::size_t p;

	p = s.find("://");
	if (p != std::string::npos)
		r.erase(0, p + 3);

	if (r.length() >= 4 &&
		::tolower(r[0]) == 'w' &&
			::tolower(r[1]) == 'w' &&
			::tolower(r[2]) == 'w' &&
			::tolower(r[3]) == '.')
		r.erase(0, 4);

	int i = 0;
	while (r[i] == '/' || r[i] == '.')
		i++;

	if (i != 0)
		r.erase(0, i);

	return r;
}

int passcave::simple_compareStringURIs(std::string const& s1, std::string const& s2) {
	std::string ss1 = simple_clearURIPrefix(s1);
	std::string ss2 = simple_clearURIPrefix(s2);

	int r = simple_compareStrings(ss1, ss2);
	secureErase(ss1);
	secureErase(ss2);

	return r;
}

inline char randChar() {
	return std::rand() % 254 + 1;
}

void passcave::secureErase(void* start, int len) {
	volatile char* p = static_cast<char*>(start);
	asm volatile("rep stosb" : "+c"(len), "+D"(p) : "a"(0) : "memory");
}

void passcave::secureErase(std::string& s) {
	secureErase(&s[0], s.length());
}

void passcave::secureErase(std::wstring& s) {
	secureErase(&s[0], s.length() * sizeof(wchar_t));
}

void passcave::secureErase(std::vector<char>& v) {
	secureErase(v.data(), v.size());
}

void passcave::secureErase(QString& s) {
	// QChar is immutable, let's do something ugly
	for (QChar& c: s)
		secureErase(reinterpret_cast<char*>(&c), sizeof(QChar));
}

std::vector<char> passcave::bzipCompress(void const* in, int inSize) {
	QByteArray inV = qCompress(reinterpret_cast<unsigned char const*>(in), inSize, 9);
	std::vector<char> r(inV.begin(), inV.end());
	secureErase(inV.begin(), inV.end() - inV.begin());
	return r;
}

std::vector<char> passcave::bzipUncompress(void const* in, int inSize) {
	QByteArray inV = qUncompress(reinterpret_cast<unsigned char const*>(in), inSize);
	std::vector<char> r(inV.begin(), inV.end());
	secureErase(inV.begin(), inV.end() - inV.begin());
	return r;
}

bool passcave::fileExists(std::string f) {
	std::ifstream file(f.c_str());
	bool r = file.good();
	file.close();
	return r;
}
