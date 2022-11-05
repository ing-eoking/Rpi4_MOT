#include "iniParser.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>


iniParser::iniParser(const std::string& path){
	std::ifstream openFile(path);
	if(openFile.is_open()){
		printf("INI File opened successfully.\n", path);
		std::string line;
		while(getline(openFile, line)){
			std::string div = " = ";
			if(std::string::npos == line.find(div)) div = "=";
			std::string t1 = line.substr(0, line.find(div));
			std::string t2 = line.substr(line.find(div)+div.length(),line.length());
			table[t1] = t2;				
		}
		openFile.close();
	}
	else printf("No File!\n", path);
}
bool iniParser::isContain(const std::string& name){
	if(table.find(name)==table.end()) return false;
	return true;
}
bool iniParser::getBool(const std::string& name){
	if(isContain(name)){
		if(table[name][0] == 't' || table[name][0] == 'T') return true;
		else return false;
	}
	else throw std::invalid_argument("Not exist Name.");
}
std::string iniParser::getString(const std::string& name){
	if(isContain(name)){
		if(table[name].find("\"") == std::string::npos) return table[name];
		else return table[name].substr(1,table[name].length() -2);	
	}
	else throw std::invalid_argument("Not exist Name.");
}
float iniParser::getFloat(const std::string& name){
	if(isContain(name)) return std::stof(table[name]);
	else throw std::invalid_argument("Not exist Name.");
}
int iniParser::getInt(const std::string& name){
	if(isContain(name)) return std::stoi(table[name]);
	else throw std::invalid_argument("Not exist Name.");
	
}
std::vector<std::string> iniParser::getStringVector(const std::string& name){
	if(isContain(name)){
		std::vector<std::string> ret;
		std::string item = table[name], buf;		
		item.erase(remove(item.begin(), item.end(), ' '), item.end());
		std::istringstream iss(item);
		while(getline(iss, buf, ',')) ret.push_back(buf);
		return ret;	
	}
	else throw std::invalid_argument("Not exist Name.");
}
