#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <map>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <codecvt>
using namespace std;

enum class GrammarType {
	Undefined,
	LeftSided,
	RightSided
};

class Grammar {
public:
	GrammarType Type = GrammarType::Undefined;
	string FinaleState;
	unordered_map<string, map<string, vector<string>>> Productions;
};

string join(const vector<string>& vec, const string& delimiter) {
	string result;
	for (size_t i = 0; i < vec.size(); ++i) {
		result += vec[i];
		if (i != vec.size() - 1) {
			result += delimiter;
		}
	}
	return result;
}

vector<string> ReadAllLines(const string& inputFile)
{
	vector<string> lines;
	ifstream file(inputFile);
	file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));

	if (!file.is_open())
	{
		throw runtime_error("error: " + inputFile);
	}
	string line;
	while (getline(file, line))
	{
		lines.push_back(line);
	}
	file.close();
	return lines;
}

vector<string> CombineLines(const vector<string>& lines)
{
	vector<string> combinedLines;
	string currentLine = "";

	regex rulePattern(R"(^\s*<(\w+)>\s*->)");

	for (const string& line : lines)
	{
		if (line.empty() || all_of(line.begin(), line.end(), ::isspace))
		{
			continue;
		}

		if (regex_search(line, rulePattern))
		{
			if (!currentLine.empty())
			{
				combinedLines.push_back(currentLine);
				currentLine.clear();
			}
			currentLine = line;
		}
		else
		{
			string text = "";
			text = regex_replace(line, regex("^\\s+"), "");
			currentLine += " " + text;
		}
	}

	if (!currentLine.empty())
	{
		combinedLines.push_back(currentLine);
	}

	return combinedLines;
}

void DetermineGrammarType(const vector<string>& lines, Grammar& grammar) {
	bool isLeftSided = true;
	bool isRightSided = true;

	regex startsWithNonterminal(R"(^<\w+>.*)");
	//regex endsWithNonterminal(R"(.*<\w+>$)");
	regex endsWithNonterminal(R"(.*<\w+>\s*$)");

	for (const string& line : lines) {
		// Разделяем строку на левую и правую части
		vector<string> parts;
		size_t pos = line.find("->");
		if (pos == string::npos) continue;

		string lhs = line.substr(0, pos); // Левая часть (нетерминал)
		string rhs = line.substr(pos + 2); // Правая часть (правила)

		// Убираем лишние пробелы
		lhs.erase(lhs.begin(), find_if(lhs.begin(), lhs.end(), [](unsigned char ch) {
			return !isspace(ch);
			}));
		lhs.erase(find_if(lhs.rbegin(), lhs.rend(), [](unsigned char ch) {
			return !isspace(ch);
			}).base(), lhs.end());

		rhs.erase(rhs.begin(), find_if(rhs.begin(), rhs.end(), [](unsigned char ch) {
			return !isspace(ch);
			}));
		rhs.erase(find_if(rhs.rbegin(), rhs.rend(), [](unsigned char ch) {
			return !isspace(ch);
			}).base(), rhs.end());

		// Разделяем правую часть на отдельные правила
		vector<string> productions;
		size_t delimiterPos = 0;
		string delimiter = "|";
		string token;
		while ((delimiterPos = rhs.find(delimiter)) != string::npos) {
			token = rhs.substr(0, delimiterPos);
			productions.push_back(token);
			rhs.erase(0, delimiterPos + delimiter.length());
		}
		productions.push_back(rhs);

		// Проверяем каждое правило
		for (const string& production : productions) {
			if (regex_match(production, startsWithNonterminal)) {
				isRightSided = false;
			}
			if (regex_match(production, endsWithNonterminal)) {
				isLeftSided = false;
			}
		}
	}

	// Определяем тип грамматики
	if (isLeftSided) {
		grammar.Type = GrammarType::LeftSided;
	}
	else if (isRightSided) {
		grammar.Type = GrammarType::RightSided;
	}
	else {
		grammar.Type = GrammarType::Undefined;
	}
}

void ParseLeftHandedGrammar(vector<string>& statesGrammar, const vector<string>& lines, Grammar& grammar) {
	regex grammarPattern(R"(^\s*<(\w+)>\s*->\s*((?:<\w+>\s+)?[\wε](?:\s*\|\s*(?:<\w+>\s+)?[\wε])*)\s*$)");
	regex transitionPattern(R"(^\s*(?:<(\w*)>)?\s*([\wε]*)\s*$)");

	wregex grammarPatternLR(LR"(^\s*<(\w+)>\s*->\s*((?:<\w+>\s+)?[\wε](?:\s*\|\s*(?:<\w+>\s+)?[\wε])*)\s*$)");

	for (const string& line : lines)
	{
		wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
		wstring wstr = converter.from_bytes(line);

		if (!regex_match(wstr, grammarPatternLR)) continue;
		wsmatch match;
		if (!regex_match(wstr, match, grammarPatternLR)) continue;

		wstring stateLR = match[1];
		string state = converter.to_bytes(stateLR);

		wstring transitionsLR = match[2];
		string transitions = converter.to_bytes(transitionsLR);

		if (grammar.Productions.find(state) == grammar.Productions.end()) {
			grammar.Productions[state] = map<string, vector<string>>();
		}

		statesGrammar.push_back(state);

		if (grammar.FinaleState.empty()) {
			grammar.FinaleState = state;
		}

		size_t pos = 0;
		while ((pos = transitions.find('|')) != string::npos)
		{
			string transition = transitions.substr(0, pos);
			transitions.erase(0, pos + 1);
			smatch transMatch;

			if (regex_match(transition, transMatch, transitionPattern))
			{
				string symbol = transMatch[2];
				string nextState = transMatch[1].matched ? transMatch[1].str() : "H";

				if (grammar.Productions.find(nextState) == grammar.Productions.end()) {
					grammar.Productions[nextState] = map<string, vector<string>>();
				}
				if (grammar.Productions[nextState].find(symbol) == grammar.Productions[nextState].end()) {
					grammar.Productions[nextState][symbol] = vector<string>();
				}
				grammar.Productions[nextState][symbol].push_back(state);
			}
		}

		smatch transMatch;
		if (regex_match(transitions, transMatch, transitionPattern))
		{
			string symbol = transMatch[2];
			string nextState = transMatch[1].matched ? transMatch[1].str() : "H";

			if (grammar.Productions.find(nextState) == grammar.Productions.end()) {
				grammar.Productions[nextState] = map<string, vector<string>>();
			}
			if (grammar.Productions[nextState].find(symbol) == grammar.Productions[nextState].end()) {
				grammar.Productions[nextState][symbol] = vector<string>();
			}
			if (nextState == "H")
			{
				statesGrammar.insert(statesGrammar.begin(), nextState);
			}
			grammar.Productions[nextState][symbol].push_back(state);
		}
	}
}

void ParseRightHandedGrammar(vector<string>& statesGrammar, const vector<string>& lines, Grammar& grammar) {
	regex grammarPattern(R"(^\s*<(\w+)>\s*->\s*([\wε](?:\s+<\w+>)?(?:\s*\|\s*[\wε](?:\s+<\w+>)?)*)\s*$)");
	regex transitionPattern(R"(^\s*([\wε]*)\s*(?:<(\w*)>)?\s*$)");

	wregex grammarPatternLR(LR"(^\s*<(\w+)>\s*->\s*([\wε](?:\s+<\w+>)?(?:\s*\|\s*[\wε](?:\s+<\w+>)?)*)\s*$)");

	const string finalState = "F";

	for (const auto& line : lines) {
		wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
		wstring wstr = converter.from_bytes(line);

		if (!regex_match(wstr, grammarPatternLR)) continue;

		wsmatch match;
		if (!regex_match(wstr, match, grammarPatternLR)) continue;

		wstring stateLR = match[1];
		string state = converter.to_bytes(stateLR);

		wstring transitionsLR = match[2];
		string transitions = converter.to_bytes(transitionsLR);

		if (grammar.Productions.find(state) == grammar.Productions.end()) {
			grammar.Productions[state] = map<string, vector<string>>();
			statesGrammar.push_back(state);
		}

		istringstream transitionsStream(transitions);
		string transition;
		while (getline(transitionsStream, transition, '|')) {
			smatch transMatch;
			if (!regex_match(transition, transMatch, transitionPattern)) continue;

			string symbol = transMatch[1];
			string nextState = transMatch[2].matched ? transMatch[2] : finalState;

			if (grammar.Productions[state].find(symbol) == grammar.Productions[state].end()) {
				grammar.Productions[state][symbol] = vector<string>();
			}
			grammar.Productions[state][symbol].push_back(nextState);
		}
	}
	grammar.Productions[finalState] = map<string, vector<string>>();
	grammar.FinaleState = finalState;
	statesGrammar.push_back(finalState);
}

void WriteToFile(const vector<string>& statesGrammar, const vector<string>& lines, const Grammar& grammar, const std::string& outputFileName) {
	string initState;
	if (grammar.Type == GrammarType::LeftSided)
	{
		initState = "H";
	}
	else if (grammar.Type == GrammarType::RightSided)
	{
		initState = statesGrammar[0];
	}
	else {
		throw invalid_argument("Grammar does not have any productions.");
	}

	set<string> symbolSet;
	for (const auto& production : grammar.Productions)
	{
		for (const auto& transition : production.second)
		{
			symbolSet.insert(transition.first);
		}
	}

	vector<string> symbols(symbolSet.begin(), symbolSet.end());
	sort(symbols.begin(), symbols.end());

	//заголовки CSV
	vector<string> header1 = { "" };
	for (const auto& state : statesGrammar) {
		if (state == grammar.FinaleState)
		{
			header1.push_back("F");
		}
		else
		{
			header1.push_back("");
		}
	}

	vector<string> header2 = { "" };
	for (size_t i = 0; i < statesGrammar.size(); ++i) {
		header2.push_back("q" + std::to_string(i));
	}

	map<string, string> stateIndexMap;
	for (size_t i = 0; i < statesGrammar.size(); ++i) {
		stateIndexMap[statesGrammar[i]] = "q" + std::to_string(i);
	}

	//Создаём строки для CSV
	vector<vector<string>> rows;
	for (const auto& symbol : symbols)
	{
		vector<string> row = { symbol };
		for (const auto& state : statesGrammar)
		{
			if (grammar.Productions.count(state) && grammar.Productions.at(state).count(symbol))
			{
				const auto& nextStates = grammar.Productions.at(state).at(symbol);
				vector<string> mappedStates;
				for (const auto& nextState : nextStates) {
					mappedStates.push_back(stateIndexMap[nextState]);
				}
				row.push_back(join(mappedStates, ","));
			}
			else
			{
				row.push_back("");
			}
		}
		rows.push_back(row);
	}

	ofstream writer(outputFileName);
	if (!writer.is_open()) {
		throw std::runtime_error("Could not open file for writing.");
	}
	writer << join(header1, ";") << std::endl;
	writer << join(header2, ";") << std::endl;
	for (const auto& row : rows) {
		writer << join(row, ";") << std::endl;
	}
}

int main(int argc, char* argv[])
{
	string inputFile = argv[1], outputFile = argv[2];
	vector<string> lines = ReadAllLines(inputFile);
	vector<string> combinedLines = CombineLines(lines);
	Grammar grammar;
	DetermineGrammarType(combinedLines, grammar);
	vector<string> statesGrammar;

	if (grammar.Type == GrammarType::LeftSided) {
		ParseLeftHandedGrammar(statesGrammar, combinedLines, grammar);
	}
	else if (grammar.Type == GrammarType::RightSided) {
		ParseRightHandedGrammar(statesGrammar, combinedLines, grammar);
	}
	else {
		throw std::runtime_error("Тип грамматики не определен");
	}

	WriteToFile(statesGrammar, combinedLines, grammar, outputFile);

	return 0;
}