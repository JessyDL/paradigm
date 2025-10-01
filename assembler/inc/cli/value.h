#pragma once
#define NO_MIN_MAX
#undef MIN
#undef MAX
#include "psl/array.hpp"
#include "psl/memory/region.hpp"
#include "psl/static_array.hpp"
#include "psl/string_utils.hpp"
#include "psl/ustring.hpp"
#include "stdafx.h"
#include <iostream>
#include <memory>
#include <optional>
#include <queue>

////namespace psl
////{
////	psl::string8_t::const_iterator find_substring_index(const psl::string8_t& str, psl::string8::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string8_t::iterator find_substring_index(psl::string8_t& str, psl::string8::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string16_t::const_iterator find_substring_index(const psl::string16_t& str, psl::string16::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string16_t::iterator find_substring_index(psl::string16_t& str, psl::string16::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string32_t::const_iterator find_substring_index(const psl::string32_t& str, psl::string32::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string32_t::iterator find_substring_index(psl::string32_t& str, psl::string32::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string8::view::iterator find_substring_index(psl::string8::view str, psl::string8::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string16::view::iterator find_substring_index(psl::string16::view str, psl::string16::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////
////	psl::string32::view::iterator find_substring_index(psl::string32::view str, psl::string32::view view)
////	{
////		if(view.data() < str.data() || view.data() >= str.data() + str.size()) return std::end(str);
////
////		return std::next(std::begin(str), view.data() - str.data());
////	}
////} // namespace psl

namespace psl::cli {
template <typename T>
class value;

class pack;

namespace details {
	static psl::static_array<psl::string8::char_t, 4> illegal_characters {' ', '"', '\'', '-'};
	using type_key_t = std::uintptr_t const* (*)();
	template <typename T>
	constexpr const std::uintptr_t type_key_var {0u};

	template <typename T>
	constexpr std::uintptr_t const* type_key() noexcept {
		return &type_key_var<T>;
	}

	template <typename T>
	constexpr type_key_t key_for() {
		return type_key<typename std::decay<T>::type>;
	};

	class value_base {
	  public:
		value_base(psl::string8_t name,
				   psl::array<psl::string8_t> const& commands,
				   bool optional,
				   type_key_t id,
				   psl::string_view descr = {})
			: m_Name(name), m_Optional(optional), m_ID(id), m_Description(descr) {
			for(auto const& command : commands) {
				validate(command);
				if(command.size() == 1) {
					m_ShortCommands.emplace_back(command[0]);
					continue;
				}
				m_Commands.emplace_back(command);
			}
		}
		virtual ~value_base() = default;
		bool optional() const noexcept {
			return m_Optional;
		};
		psl::string8_t const& name() const noexcept {
			return m_Name;
		}
		psl::string8_t name() noexcept {
			return m_Name;
		}
		bool contains_command(psl::string8::view command, bool include_short_commands = true) const noexcept {
			return std::find(std::begin(m_Commands), std::end(m_Commands), command) != std::end(m_Commands) ||
				   (include_short_commands && command.size() == 1 && contains_short_command(command[0]));
		}
		bool contains_short_command(psl::string8::char_t command) const noexcept {
			return std::find(std::begin(m_ShortCommands), std::end(m_ShortCommands), command) !=
				   std::end(m_ShortCommands);
		}

		template <typename T>
		value<T>& as() {
			if(m_ID != key_for<T>())
				throw std::runtime_error("incorrect cast");

			return *dynamic_cast<value<T>*>(this);
		}

		template <typename T>
		bool is_a() const noexcept {
			return m_ID == key_for<T>();
		}
		bool conflicts(value_base const& other) const noexcept {
			auto cmds		= commands();
			auto other_cmds = other.commands();

			std::sort(std::begin(cmds), std::end(cmds));
			std::sort(std::begin(other_cmds), std::end(other_cmds));
			psl::array<psl::string8_t> res;
			std::set_intersection(
			  std::begin(cmds), std::end(cmds), std::begin(other_cmds), std::end(other_cmds), std::back_inserter(res));
			return res.size() > 0;
		}

		virtual bool from_string(psl::string_view t) = 0;
		virtual psl::string8_t valid_arguments()	 = 0;

		psl::string_view description() const noexcept {
			return m_Description;
		}
		void description(psl::string_view descr) {
			m_Description = descr;
		};
		psl::array<psl::string8_t> commands() const noexcept {
			auto cpy = m_Commands;
			std::for_each(std::begin(m_ShortCommands),
						  std::end(m_ShortCommands),
						  [&cpy](psl::string8::char_t character) { cpy.emplace_back(psl::string8_t(&character, 1)); });
			return cpy;
		}

		virtual void reset() = 0;

	  private:
		void validate(psl::string8_t const& command) {
			std::for_each(
			  std::begin(illegal_characters), std::end(illegal_characters), [&command](psl::string8::char_t character) {
				  if(std::find(std::begin(command), std::end(command), character) != std::end(command))
					  throw std::runtime_error("invalid character in command, a '" + psl::string8_t(&character, 1) +
											   "' was detected");
			  });
		}

		psl::string8_t m_Name;
		psl::string8_t m_Description;
		psl::array<psl::string8_t> m_Commands;
		psl::array<psl::string8::char_t> m_ShortCommands;
		bool m_Optional;
		type_key_t m_ID;
	};

	template <typename T>
	struct is_vector : std::false_type {
		using type = T;
	};
	template <typename T, typename... Ts>
	struct is_vector<std::vector<T, Ts...>> : std::true_type {
		using type = T;
	};
}	 // namespace details

template <typename T>
class value final : public details::value_base {
	using internal_type = typename details::is_vector<T>::type;

  public:
	value(psl::string8_t name,
		  psl::array<psl::string8_t> const& commands,
		  std::optional<std::shared_ptr<T>> default_value		   = std::nullopt,
		  bool optional											   = true,
		  std::optional<std::vector<internal_type>> allowed_values = std::nullopt)
		: details::value_base(name, commands, optional, details::key_for<T>()), m_Default(default_value),
		  m_AllowedValues(allowed_values) {};
	value(psl::string8_t name,
		  psl::array<psl::string8_t> const& commands,
		  T default_value,
		  bool optional											   = true,
		  std::optional<std::vector<internal_type>> allowed_values = std::nullopt)
		: details::value_base(name, commands, optional, details::key_for<T>()),
		  m_Default(std::make_shared<T>(default_value)), m_AllowedValues(allowed_values) {};

	value(psl::string8_t name,
		  psl::string_view description,
		  psl::array<psl::string8_t> const& commands,
		  std::optional<std::shared_ptr<T>> default_value		   = std::nullopt,
		  bool optional											   = true,
		  std::optional<std::vector<internal_type>> allowed_values = std::nullopt)
		: details::value_base(name, commands, optional, details::key_for<T>(), description), m_Default(default_value),
		  m_AllowedValues(allowed_values) {};
	value(psl::string8_t name,
		  psl::string_view description,
		  psl::array<psl::string8_t> const& commands,
		  T default_value,
		  bool optional											   = true,
		  std::optional<std::vector<internal_type>> allowed_values = std::nullopt)
		: details::value_base(name, commands, optional, details::key_for<T>(), description),
		  m_Default(std::make_shared<T>(default_value)), m_AllowedValues(allowed_values) {};

	bool has_value() const noexcept {
		return m_Value || m_Default;
	}

	const T& get() const {
		if(m_Value)
			return *(m_Value.value());
		if(m_Default)
			return *(m_Default.value());

		throw std::runtime_error("no value found");
	}

	std::shared_ptr<T> get_shared() {
		if(!m_Value && m_Default) {
			m_Value = std::make_shared<T>(*m_Default.value());
		}

		if(!m_Value)
			throw std::runtime_error("no value found");

		return m_Value.value();
	}

	void set(const T& val) {
		if(!m_Value) {
			m_Value = std::make_shared<T>(val);
		} else {
			*(m_Value.value()) = val;
		}
	}

	void reset() override {
		if constexpr(std::is_same<bool, T>::value)
			set((m_Default) ? !(*m_Default.value()) : true);
		else {
			if(m_Default)
				set(*m_Default.value());
			else
				set({});
		}
	}

	void clear() {
		m_Value = std::nullopt;
	}

	void set_default(const T& val) {
		m_Default = std::make_shared<T>(val);
	}
	void set_default(std::shared_ptr<T>& val) {
		m_Default = val;
	}

	value<T> copy(bool linked_default = false) const noexcept {
		value<T> cpy {
		  name(),
		  commands(),
		  ((linked_default) ? m_Default : ((m_Default) ? m_Default.value() : std::optional<std::shared_ptr<T>> {})),
		  optional()};
		if(m_Value)
			cpy.set(*m_Value.value());
		return cpy;
	}

	bool from_string(psl::string_view string) override {
		if constexpr(!std::is_same<pack, T>::value) {
			if(string.size() == 0) {
				if constexpr(std::is_same<bool, T>::value) {
					set((m_Default) ? !(*m_Default.value()) : true);
					return true;
				}
			}

			if constexpr(details::is_vector<T>::value) {
				auto res = utility::string::split(string, " ");
				T val;
				val.reserve(res.size());
				for(auto const& entry : res) {
					val.emplace_back(utility::converter<internal_type>::from_string(psl::to_string8_t(entry)));
					if(!valid(val[val.size() - 1]))
						return false;
				}
				set(val);
			} else {
				auto res = utility::converter<internal_type>::from_string(psl::to_string8_t(string));
				if(!valid(res))
					return false;
				set(res);
			}
			return true;
		}
		return false;
	}
	virtual psl::string8_t valid_arguments() override {
		if constexpr(!std::is_same<pack, T>::value) {
			if(!m_AllowedValues.has_value() || m_AllowedValues.value().size() == 0)
				return "";

			psl::string8_t res;
			for(auto const& value : m_AllowedValues.value()) {
				res += utility::converter<internal_type>::to_string(value) + ", ";
			}
			res.resize(res.size() - 2);

			auto last_index = res.find_last_of(',');
			if(last_index != psl::string::npos) {
				res.erase(last_index, 1);
				res.insert(last_index, " or");
			}
			return res;
		} else {
			return "";
		}
	}

  private:
	bool valid(internal_type const& value) {
		if(!m_AllowedValues.has_value() || m_AllowedValues.value().size() == 0)
			return true;

		return std::any_of(std::begin(m_AllowedValues.value()),
						   std::end(m_AllowedValues.value()),
						   [&value](auto const& v) { return v == value; });
	}

	std::optional<std::vector<internal_type>> m_AllowedValues;
	std::optional<std::shared_ptr<T>> m_Value;
	std::optional<std::shared_ptr<T>> m_Default;
};

class pack final {
  public:
	template <typename Fn,
			  typename... Ts,
			  typename = std::enable_if_t<!(std::is_same<pack, std::decay_t<Fn>>::value && sizeof...(Ts) == 0)>>
	pack(Fn&& fn, Ts&&... values) {
		if constexpr(std::is_invocable<Fn, pack&>::value || std::is_bind_expression<Fn>::value) {
			m_Callback = std::function<void(psl::cli::pack&)>(fn);
		} else {
			add(std::forward<Fn>(fn));
		}
		(add(std::forward<Ts>(values)), ...);
	}


	template <typename... Ts>
	pack(std::function<void(psl::cli::pack&)>&& fn, Ts&&... values) {
		m_Callback = fn;
		(add(std::forward<Ts>(values)), ...);
	}
	pack()						 = default;
	~pack()						 = default;
	pack(pack const&)			 = default;
	pack(pack&&)				 = default;
	pack& operator=(pack const&) = default;
	pack& operator=(pack&&)		 = default;

	auto begin() noexcept {
		return std::begin(m_Values);
	}
	auto end() noexcept {
		return std::end(m_Values);
	}
	auto cbegin() const noexcept {
		return std::cbegin(m_Values);
	}
	auto cend() const noexcept {
		return std::cend(m_Values);
	}

	std::shared_ptr<details::value_base> operator[](psl::string8_t const& name) const {
		for(auto const& v : m_Values) {
			if(v->name() == name)
				return v;
		}
		throw std::runtime_error("could not find value");
	}

  private:
	size_t next_instance_of(psl::string_view view, psl::char_t delimiter, size_t offset = 0) const noexcept {
		offset = view.find(delimiter, offset);
		while(offset != view.npos) {
			if(offset == 0 || (offset > 0 && view[offset - 1] != '\\'))
				return offset;
			offset = view.find(delimiter, offset + 1);
		}
		return offset;
	}

	bool occupied(size_t index, psl::array<std::pair<size_t, size_t>> const& range) const noexcept {
		return std::any_of(std::begin(range), std::end(range), [index](const std::pair<size_t, size_t> r) {
			return r.first <= index && index <= r.second;
		});
	}

	struct command {
		psl::string_view cmd;
		psl::string_view args;
		bool is_long_command {false};
	};

  public:
	void parse(psl::array<psl::string_view> commands) {
		reset();
		if(commands.size() == 0 ||
		   std::all_of(std::begin(commands), std::end(commands), [](auto const& command) { return command.empty(); })) {
			assembler::log->warn("no commands entered...");
			return;
		}

		for(auto const& view : commands) {
			if(view.empty())
				continue;
			// todo: support the ' delimiter
			psl::array<std::pair<size_t, size_t>> occupied_ranges {};

			for(size_t token_offset = next_instance_of(view, '"', 0); token_offset != psl::string_view::npos;
				token_offset		= next_instance_of(view, '"', token_offset + 1)) {
				auto begin_token = token_offset + 1;
				token_offset	 = next_instance_of(view, '"', begin_token);
				if(token_offset == psl::string_view::npos) {
					// error out
					assembler::log->error("ERROR: opened a delimiter (\"), but did not close it again.");
					return;
				}
				occupied_ranges.emplace_back(std::pair {begin_token, token_offset});
			}

			psl::array<command> commands {};
			for(size_t token_offset = view.find('-'); token_offset != psl::string_view::npos;
				token_offset		= view.find('-', token_offset + 1)) {
				if(occupied(token_offset, occupied_ranges))
					continue;
				bool is_long_command = (token_offset != view.size() - 1) && view[token_offset + 1] == '-';
				token_offset += (is_long_command) ? 2 : 1;

				size_t cmd_start = token_offset;
				size_t cmd_end	 = view.find(' ', cmd_start);
				size_t arg_start = view.find_first_not_of(" \"", cmd_end);
				size_t arg_end	 = view.find('-', token_offset);

				cmd_end	  = (cmd_end == view.npos) ? view.size() : cmd_end;
				arg_start = (arg_start == view.npos) ? view.size() : arg_start;

				while(arg_end != psl::string_view::npos && occupied(arg_end, occupied_ranges)) {
					arg_end = view.find('-', arg_end + 1);
				}
				if(arg_end == psl::string_view::npos)
					arg_end = view.size();

				if(auto temp_end = utility::string::rfind_first_not_of(view, " \"", arg_end); temp_end >= arg_start)
					arg_end = temp_end;

				if(!is_long_command && cmd_end - cmd_start > 1) {
					assembler::log->error(
					  "ERROR: a short command cannot have multiple characters, please check: - {}",
					  psl::to_string8_t(psl::string_view(view.data() + cmd_start, cmd_end - cmd_start)));
					return;
				}
				if(!is_long_command && cmd_end - cmd_start == 0) {
					assembler::log->error(
					  "ERROR: a short command supplied, but no character behind it, please check the "
					  "free floating '-'");
					return;
				}
				if(cmd_end != cmd_start) {
					commands.emplace_back(command {psl::string_view(view.data() + cmd_start, cmd_end - cmd_start),
												   psl::string_view(view.data() + arg_start, arg_end - arg_start),
												   is_long_command});
				}
			}

			if(!validate(commands, true))
				return;

			std::queue<pack*> processed_packs;
			bool error = false;
			parse(std::begin(commands), std::end(commands), processed_packs, error);

			if(error) {
				return;
			}
			while(processed_packs.size() > 0) {
				processed_packs.front()->operator()();
				processed_packs.pop();
			}
		}
	}

	void callback(std::function<void(pack&)> fn) {
		m_Callback = fn;
	}

	void operator()() {
		if(m_Callback) {
			std::invoke(m_Callback.value(), *this);
		}
	}

	void print_help(size_t depth = 0) const noexcept {
		assembler::log->info(psl::string8_t(80, '-'));
		for(auto const& v : m_Values) {
			assembler::log->info(psl::string8_t(depth, '\t') + psl::to_string8_t(v->name()) +
								 ((v->optional()) ? "* " : " ") + psl::to_string8_t(v->description()));
		}
		assembler::log->info(psl::string8_t(80, '-'));
	}

  private:
	bool validate(psl::array<command> const& commands, bool print_reason = false) {
		bool failed = false;
		internal_validate(std::begin(commands), std::end(commands), failed, true, print_reason);
		return !failed;
	}

	bool internal_validate(psl::array<psl::array<command>::const_iterator> const& values,
						   bool& failed,
						   bool print_reason = false) const noexcept {
		for(auto const& v : m_Values) {
			if(!v->optional() &&
			   std::none_of(std::begin(values), std::end(values), [v](psl::array<command>::const_iterator it) {
				   return v->contains_command(it->cmd);
			   })) {
				if(print_reason)
					assembler::log->error("ERROR: missing required command '{}'", psl::to_string8_t(v->name()));
				failed = true;
				return false;
			}
		}
		return true;
	}

	psl::array<command>::const_iterator internal_validate(psl::array<command>::const_iterator begin,
														  psl::array<command>::const_iterator end,
														  bool& failed,
														  bool root			= false,
														  bool print_reason = false) const noexcept {
		psl::array<psl::array<command>::const_iterator> owned_values {};
		for(auto command_it = begin; command_it != end; command_it = std::next(command_it)) {
			if(is_help(*command_it))
				continue;

			auto value = get_value(*command_it);
			if(!value) {
				if(root) {
					assembler::log->error("ERROR: invalid command detected. the command '{}' was not found.\n",
										  psl::to_string8_t(command_it->cmd));
					failed = true;
				}
				return internal_validate(owned_values, failed, print_reason) ? command_it : end;
			}
			if(value->is_a<cli::pack>()) {
				pack* new_child = value->as<cli::pack>().get_shared().operator->();
				if(new_child != this) {
					command_it =
					  std::prev(new_child->internal_validate(std::next(command_it), end, failed, false, print_reason));
				}
			} else {
				owned_values.emplace_back(command_it);
			}
		}

		internal_validate(owned_values, failed, print_reason);
		return end;
	}

	psl::string8_t to_string(psl::array<psl::string8_t> const& strs) const noexcept {
		psl::string8_t res {};
		for(auto const& str : strs) res.append(str + "; ");
		return res;
	}

	void internal_print_help(size_t depth = 0) {
		size_t col_1 = 32;
		size_t col_2 = 32;
		if(depth == 0) {
			assembler::log->info("name" + psl::string8_t(col_1 - 4, ' ') + "| commands" +
								 psl::string8_t(col_2 - 8, ' ') + "| description");
			assembler::log->info(psl::string8_t(128, '-'));
		}
		for(auto const& v : m_Values) {
			auto commands = to_string(v->commands());

			size_t padding_1 = depth * 2 + v->name().size() + (v->optional() ? 1 : 2);
			padding_1		 = (padding_1 >= col_1) ? 1 : col_1 - padding_1;
			size_t padding_2 = (commands.size() >= col_2) ? 1 : col_2 - commands.size();
			assembler::log->info(psl::string8_t(depth * 2, ' ') + psl::to_string8_t(v->name()) +
								 ((v->optional()) ? " " : "* ") + psl::string8_t(padding_1, ' ') + "| " +
								 psl::to_string8_t(commands) + psl::string8_t(padding_2, ' ') + "| " +
								 psl::to_string8_t(v->description()));
			if(v->is_a<cli::pack>()) {
				pack* new_child = v->as<cli::pack>().get_shared().operator->();
				if(new_child != this) {
					new_child->internal_print_help(depth + 1);
				}
			}
		}
	}

	bool has_command(psl::string_view cmd) {
		return std::any_of(std::begin(m_Values),
						   std::end(m_Values),
						   [&cmd](std::shared_ptr<details::value_base> const& v) { return v->contains_command(cmd); });
	}

	bool is_pack(psl::string_view cmd) {
		return std::any_of(
		  std::begin(m_Values), std::end(m_Values), [&cmd](std::shared_ptr<details::value_base> const& v) {
			  return v->contains_command(cmd) && v->is_a<cli::pack>();
		  });
	}

	std::shared_ptr<details::value_base> get_value(command const& cmd) const noexcept {
		auto index =
		  std::find_if(std::begin(m_Values), std::end(m_Values), [&cmd](std::shared_ptr<details::value_base> const& v) {
			  return v->contains_command(cmd.cmd);
		  });
		if(index == std::end(m_Values))
			return nullptr;
		return *index;
	}

	bool is_help(command const& cmd) const noexcept {
		return cmd.cmd == "help" || cmd.cmd == "h";
	}

	void reset() {
		for(auto& value : m_ModifiedValues) {
			if(value->is_a<cli::pack>()) {
				pack* new_child = value->as<cli::pack>().get_shared().operator->();
				new_child->reset();
			} else {
				value->reset();
			}
		}
		m_ModifiedValues.clear();
	}

	psl::array<command>::const_iterator parse(psl::array<command>::const_iterator begin,
											  psl::array<command>::const_iterator end,
											  std::queue<pack*>& processed_packs,
											  bool& error) {
		processed_packs.push(this);

		for(auto command_it = begin; command_it != end; command_it = std::next(command_it)) {
			if(is_help(*command_it)) {
				internal_print_help();
				continue;
			}

			auto value = get_value(*command_it);
			if(!value)
				return command_it;

			if(value->is_a<cli::pack>()) {
				pack* new_child = value->as<cli::pack>().get_shared().operator->();
				if(new_child != this) {
					command_it = std::prev(new_child->parse(std::next(command_it), end, processed_packs, error));
					m_ModifiedValues.emplace_back(value);
				}
			} else {
				if(!value->from_string(command_it->args)) {
					assembler::log->error(
					  "ERROR: an invalid argument was passed into '{0}'. please check '{1}'\n\tthe only allowed "
					  "values are '{2}'.",
					  psl::to_string8_t(value->name()),
					  psl::to_string8_t(command_it->args),
					  psl::to_string8_t(value->valid_arguments()));
					error = true;
					return end;
				}
				m_ModifiedValues.emplace_back(value);
			}
			// psl::cout << '\t' << "command: " << psl::to_platform_string(command_it->cmd)
			//		  << " depth: " << psl::to_platform_string(std::to_string(stack.size() - 1))
			//		  << "\n\t\t argument: " << psl::to_platform_string(command_it->args) << "\n";
		}
		return end;
	}

	void validate(details::value_base const& new_value) const {
		if(std::any_of(std::begin(m_Values), std::end(m_Values), [&new_value](auto const& value) {
			   return new_value.conflicts(*value.get());
		   }))
			throw std::runtime_error("conflicting commands found");

		if(std::any_of(std::begin(m_Values), std::end(m_Values), [&new_value](auto const& value) {
			   return new_value.name() == value->name();
		   }))
			throw std::runtime_error("conflicting name found");
	}

	template <typename T>
	void add(std::shared_ptr<value<T>>& val) {
		validate(*val);
		m_Values.emplace_back(val);
	}


	template <typename T>
	void add(value<T>&& val) {
		validate(val);
		m_Values.emplace_back(std::make_shared<value<T>>(std::move(val)));
	}

	template <typename T>
	void add(value<T> const& val) {
		validate(val);
		m_Values.emplace_back(std::make_shared<value<T>>(val));
	}
	psl::array<std::shared_ptr<details::value_base>> m_Values;
	psl::array<std::shared_ptr<details::value_base>> m_ModifiedValues;
	std::optional<std::function<void(pack&)>> m_Callback;
};
}	 // namespace psl::cli