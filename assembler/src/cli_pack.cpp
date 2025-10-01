#include "cli_pack.h"
#include "stdafx.h"
#include "terminal_utils.h"
using namespace cli;
uint64_t value<parameter_pack>::m_ID {value_base::ID_GENERATOR++};

bool parameter_pack::is_valid() const {
	return m_Valid && !m_Dirty;
}
bool parameter_pack::validate() {
	if(is_valid())
		return true;

	m_Valid = false;
	m_Dirty = false;
	m_FlattenedValues.clear();
	m_Callbacks.clear();
	if(m_Callback)
		m_Callbacks.push_back(m_Callback.value());

	for(auto const& value : m_Values) {
		if(value->is_parameter_pack() && value->name().empty() && !value->short_command() && value->command().empty()) {
			auto* ppack = value->as_a<parameter_pack>();
			ppack->get().validate();
			m_FlattenedValues.insert(std::end(m_FlattenedValues),
									 std::begin(ppack->get().m_FlattenedValues),
									 std::end(ppack->get().m_FlattenedValues));
			m_Callbacks.insert(
			  std::end(m_Callbacks), std::begin(ppack->get().m_Callbacks), std::end(ppack->get().m_Callbacks));
		} else {
			m_FlattenedValues.emplace_back(value);
		}
	}

	for(auto const& value : m_FlattenedValues) {
		auto it = std::find_if(
		  std::begin(m_FlattenedValues),
		  std::end(m_FlattenedValues),
		  [&value](std::shared_ptr<cli::value_base> const& entry) {
			  return entry != value && (entry->command() == value->command() ||
										(entry->short_command() && entry->short_command() == value->short_command()));
		  });
		if(it != std::end(m_FlattenedValues)) {
			utility::terminal::set_color(utility::terminal::color::RED);
			psl::cerr << psl::to_platform_string("duplicate parameter found: the param '" + (*it)->name() +
												 "' was equal to the param '" + value->name());
			utility::terminal::set_color(utility::terminal::color::WHITE);
			return false;
		}
	}

	m_Valid = true;
	return m_Valid;
}

bool parameter_pack::parse(std::vector<token> const& tokens, std::vector<cli::parameter_pack*>& parsed_packs) {
	if(!validate()) {
		utility::terminal::set_color(utility::terminal::color::RED);
		psl::cerr << _T("tried to parse an invalide parameter_pack, please check the output for the error message.\n");
		utility::terminal::set_color(utility::terminal::color::WHITE);
		return false;
	}

	bool missing_requirement = false;

	for(auto const& v : m_FlattenedValues) {
		if(v->required()) {
			auto it = std::find_if(std::begin(tokens), std::end(tokens), [&v](token const& tokenPair) {
				return (tokenPair.key.size() == 1 && tokenPair.key[0] == v->short_command()) ||
					   tokenPair.key == v->command();
			});
			if(it == std::end(tokens)) {
				missing_requirement = true;
				utility::terminal::set_color(utility::terminal::color::RED);
				psl::cerr << psl::to_platform_string("missing required parameter: " + psl::string(v->name()) + "\n");
				utility::terminal::set_color(utility::terminal::color::WHITE);
			}
		}
	}
	if(missing_requirement) {
		utility::terminal::set_color(utility::terminal::color::RED);
		psl::cerr << ("see the help for correct usage: \n");
		utility::terminal::set_color(utility::terminal::color::WHITE);
		psl::cout << psl::to_platform_string(help());
		return false;
	}


	m_ParsedValues.clear();
	parsed_packs.emplace_back(this);

	std::vector<cli::value_base*> parsed;

	for(auto i = 0u; i < tokens.size(); ++i) {
		auto const& token_v = tokens[i];
		auto it				= std::find_if(std::begin(m_FlattenedValues),
							   std::end(m_FlattenedValues),
							   [&token_v](std::shared_ptr<value_base> const& value) {
								   return (token_v.key.size() == 1 && token_v.key[0] == value->short_command()) ||
										  token_v.key == value->command();
							   });

		if(it != std::end(m_FlattenedValues)) {
			if((*it)->is_parameter_pack()) {
				parameter_pack& next_pack = (*((std::shared_ptr<cli::value<cli::parameter_pack>>*)&(*it)))->get();

				next_pack.m_DontInvokeCB = true;
				auto success =
				  next_pack.parse(std::vector<token> {std::begin(tokens) + i + 1, std::end(tokens)}, parsed_packs);
				next_pack.m_DontInvokeCB = false;
				if(!success)
					return false;

				if(!next_pack.fallthrough())
					break;
			} else {
				parsed.push_back(&*(*it));
				if(!(*it)->set(token_v.value.value_or(("")))) {
					utility::terminal::set_color(utility::terminal::color::RED);
					psl::cerr << psl::to_platform_string("missing parameter for: " + psl::string((*it)->name()) + '\n');
					utility::terminal::set_color(utility::terminal::color::WHITE);
					psl::cerr << psl::to_platform_string((*it)->help());
					break;
				}
			}

			m_ParsedValues.emplace_back(*it);

			if(!mixed_mode())
				break;
		} else {
			if(warn_on_unknown_parameter()) {
				utility::terminal::set_color(utility::terminal::color::RED);
				psl::cerr << psl::to_platform_string("unknown parameter: '" +
													 psl::string((token_v.key.size() > 1) + 1, '-') +
													 psl::string(token_v.key) + "' detected\n");
				utility::terminal::set_color(utility::terminal::color::WHITE);
			}
		}
	}

	for(auto const& value : m_FlattenedValues) {
		if(value->is_parameter_pack() || std::find(std::begin(parsed), std::end(parsed), &*value) != std::end(parsed)) {
			continue;
		}
		value->reset();
	}

	if(!m_DontInvokeCB) {
		std::reverse(std::begin(parsed_packs), std::end(parsed_packs));

		for(auto& pack : parsed_packs) {
			if(pack->m_Callback)
				std::invoke(pack->m_Callback.value(), *pack);
		}
	}
	return true;
}

bool parameter_pack::parse(psl::string_view parameters) {
	std::vector<token> tokens;

	auto next_parameter_n = parameters.find('-');
	while(next_parameter_n != psl::string_view::npos && next_parameter_n < parameters.size()) {
		next_parameter_n = parameters.find_first_not_of('-', next_parameter_n);
		if(next_parameter_n == psl::string_view::npos || next_parameter_n >= parameters.size())
			break;
		size_t end_parameter_n = parameters.find(' ', next_parameter_n);
		if(end_parameter_n == psl::string_view::npos || end_parameter_n >= parameters.size())
			end_parameter_n = parameters.size();
		tokens.emplace_back(parameters.substr(next_parameter_n, end_parameter_n - next_parameter_n));

		auto next_param_n = parameters.find_first_not_of(' ', end_parameter_n);
		if(next_param_n == psl::string_view::npos || next_param_n >= parameters.size() ||
		   parameters[next_param_n] == '-') {
			next_parameter_n = next_param_n;
			continue;
		}
		size_t next_param_end_n = 0u;
		// literal track
		if(parameters[next_param_n] == '\"' || parameters[next_param_n] == '\'') {
			auto is_double_quotes = (parameters[next_param_n] == '\"');
			next_param_n += 1;
			next_param_end_n = parameters.find((is_double_quotes) ? '\"' : '\'', next_param_n);
		} else {
			next_param_end_n = parameters.find(' ', next_param_n);
		}

		if(next_param_end_n == psl::string_view::npos || next_param_end_n >= parameters.size())
			next_param_end_n = parameters.size();
		tokens[tokens.size() - 1].value = parameters.substr(next_param_n, next_param_end_n - next_param_n);
		next_parameter_n				= parameters.find('-', next_param_end_n);
	}
	std::vector<cli::parameter_pack*> parsed_pack;
	return parse(tokens, parsed_pack);
}
