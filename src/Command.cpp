#include "Command.hpp"

Command::Command(const string& raw) {
	string	line = raw;
	while (!line.empty()
		&& (line[line.size() - 1] == '\n'
		|| line[line.size() - 1] == '\r'))
		line.erase(line.size() - 1);

	size_t	pos = line.find_first_not_of(' ');
	if (pos == string::npos)
		return;

	size_t	end = line.find(' ', pos);
	if (end == string::npos) {
		_name = line.substr(pos);
		return;
	}

	_name = line.substr(pos, end - pos);
	pos = end;

	while (pos < line.size()) {
		while (pos < line.size() && line[pos] == ' ')
			pos++;
		if (pos >= line.size())
			break;
		if (line[pos] == ':') {
			_args.push_back(line.substr(pos + 1));
			break;
		}
		end = line.find(' ', pos);
		if (end == string::npos) {
			_args.push_back(line.substr(pos));
			break;
		}
		_args.push_back(line.substr(pos, end - pos));
		pos = end;
	}
}

const string&	Command::getName() const {
	return _name;
}

const vector<string>&	Command::getArgs() const {
	return _args;
}
