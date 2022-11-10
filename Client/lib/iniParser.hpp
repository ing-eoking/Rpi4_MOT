#pragma once

#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <typeinfo>
#include <algorithm>

 
class iniParser{
	public:
		iniParser(const std::string& path);
		bool isSuccess() {return table.size() != 0;}
		bool isContain(const std::string& name);
		bool getBool(const std::string& name);
		std::string getString(const std::string& name);
		float getFloat(const std::string& name);
		int getInt(const std::string& name);
		std::vector<std::string> getStringVector(const std::string& name);
		template <typename T>
		std::vector<T> getNumVector(const std::string& name){
			if(isContain(name)){
				std::vector<T> ret;
				std::string item = table[name], buf;		
				item.erase(remove(item.begin(), item.end(), ' '), item.end());
				std::istringstream iss(item);
				while(getline(iss, buf, ',')){
					if(typeid(T) == typeid(int)) ret.push_back(std::stoi(buf));
					else if(typeid(T) == typeid(float)) ret.push_back(std::stof(buf));
					else if(typeid(T) == typeid(bool))
						if(buf[0] == 't' || buf[0] == 'T') ret.push_back(true);
						else ret.push_back(false);	
				}
			return ret;
		}
		else throw std::invalid_argument("Not exist Name.");
		}
	private:
		std::map<std::string, std::string> table;

};
 
