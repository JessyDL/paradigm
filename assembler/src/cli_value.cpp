#include "cli_value.h"
#include "stdafx.h"

using namespace cli;
uint64_t value_base::ID_GENERATOR {0u};
uint64_t value<bool>::m_ID {value_base::ID_GENERATOR++};

value_base::value_base(psl::string const& name,
					   psl::string const& cmd,
					   std::optional<psl::char_t> short_cmd,
					   std::optional<psl::string> hint,
					   bool required)
	: m_Name(name), m_Cmd(cmd), m_ShortCmd(short_cmd), m_Hint(hint), m_Required(required) {}

psl::string value_base::help(uint32_t indent) const {
	psl::string res;
	uint64_t cmd_size = ((m_ShortCmd) ? 2u : 0u) + 4 + m_Cmd.size();
	res += psl::string(indent * 2, ' ') + m_Name + ((required()) ? "*" : "") +
		   psl::string(24u - m_Name.size() - (uint64_t)required(), ' ') + "[" +
		   ((m_ShortCmd) ? "-" + psl::string(1u, m_ShortCmd.value()) + ", --" : "--") + m_Cmd + ']' +
		   psl::string(24 - cmd_size, ' ') + "|  " + m_Hint.value_or("") + '\n';
	return res;
}

value_base& cli::value_base::required(bool value) {
	m_Required = value;
	return *this;
}

value_base& cli::value_base::name(psl::string_view value) {
	m_Name = value;
	return *this;
}

value_base& cli::value_base::command(psl::string_view value) {
	m_Cmd = value;
	return *this;
}

value_base& cli::value_base::short_command(psl::char_t value) {
	m_ShortCmd = value;
	return *this;
}

value_base& cli::value_base::hint(psl::string_view value) {
	m_Hint = value;
	return *this;
}

bool cli::value_base::required() const {
	return m_Required;
}

psl::string_view cli::value_base::name() const {
	return m_Name;
}

psl::string_view cli::value_base::command() const {
	return m_Cmd;
}

std::optional<psl::char_t> cli::value_base::short_command() const {
	return m_ShortCmd;
}

std::optional<psl::string_view> cli::value_base::hint() const {
	return m_Hint;
}
