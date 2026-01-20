#pragma once

#include "json.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct FSMActionParamDef
{
	std::string    name;
	std::string    type;
	nlohmann::json defaultValue;
	bool           required = false;
};

struct FSMActionDef
{
	std::string					   id;
	std::string					   category;
	std::vector<FSMActionParamDef> params;
};

class FSMActionRegistry
{
public:
	static FSMActionRegistry& Instance();

	void RegisterAction(const FSMActionDef& def);
	const FSMActionDef* FindAction(const std::string& id) const;
	const std::vector<FSMActionDef>& GetActions() const;

private:
	std::vector<FSMActionDef>			    m_Actions;
	std::unordered_map<std::string, size_t> m_ActionIndex;
};

