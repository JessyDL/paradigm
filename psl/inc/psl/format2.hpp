#pragma once
#include "ustring.hpp"

namespace psl::format
{
namespace constants
{
	static const psl::string8_t EMPTY_CHARACTERS = " \n\t\r";
	static const psl::string8_t SCOPE_OPEN		 = "{";
	static const psl::string8_t SCOPE_CLOSE		 = "};";
	static const psl::string8_t DEFINITION		 = "definition";

	static const psl::string8_t LITERAL_OPEN  = "'''";
	static const psl::string8_t LITERAL_CLOSE = LITERAL_OPEN;

	static const psl::string8_t RANGE_OPEN	  = "{";
	static const psl::string8_t RANGE_CLOSE	  = "}";
	static const psl::string8_t RANGE_DIVIDER = ",";

	static const psl::string8_t REFERENCE		  = "&";
	static const psl::string8_t REFERENCE_MISSING = "null";
	static const psl::string8_t NAMESPACE_DIVIDER = "::";
}	 // namespace constants

class parser
{
  public:
	parser()  = default;
	~parser() = default;

	parser(const parser& other)				   = default;
	parser(parser&& other) noexcept			   = default;
	parser& operator=(const parser& other)	   = default;
	parser& operator=(parser&& other) noexcept = default;

  private:
};

/// \brief checks for validity of the format file
///
/// Running this on a target will verify if it is following the rules correctly
/// and if not, which rules it breaks
class validator
{
  public:
	validator()	 = default;
	~validator() = default;

	validator(const validator& other)				 = default;
	validator(validator&& other) noexcept			 = default;
	validator& operator=(const validator& other)	 = default;
	validator& operator=(validator&& other) noexcept = default;

  private:
};

enum class response
{
	ALLOW = 0,
	WARN  = 1,
	ERROR = 2
};

enum class inheritance_mode
{
	merge  = 0,
	append = 1,
};

enum class attribute
{
	optional = 0,
	required = 1
};

struct settings
{
  public:
	uint16_t major;
	uint16_t minor;
	uint16_t patch;

	bool validation_required {true};
	response dynamic_objects {response::ALLOW};	   // controls wether dynamic objects can be created, or if they
												   // all need to have a definition

	response dynamic_fields {response::ERROR};		  // controls wether _defined_ objects can have dynamic fields,
													  // i.e. fields that were never defined
	attribute field_default {attribute::optional};	  // should the field's default attribute be optional (i.e. not
													  // required), or required (i.e. error on empty/undefined)
	attribute type_default {attribute::optional};	  // should the type be written out or not
	bool nullable_references = true;				  // should we allow nullable references
  private:
};

class container
{
  public:
	container()	 = default;
	~container() = default;

	container(const container& other)				 = default;
	container(container&& other) noexcept			 = default;
	container& operator=(const container& other)	 = default;
	container& operator=(container&& other) noexcept = default;

  private:
};

enum class type : uint8_t
{
	MALFORMED = 1 << 0,
	VALUE	  = 1 << 1,
	OBJECT	  = 1 << 2,
	REFERENCE = 1 << 3,
	RANGE	  = 1 << 4
};

namespace details
{
	template <type T>
	static bool is_check(type t) noexcept
	{
		using underlaying_type = typename std::underlying_type<type>::type;
		constexpr static underlaying_type check {static_cast<underlaying_type>(T)};

		return static_cast<underlaying_type>(t) & check == check;
	}
}	 // namespace details

static bool is_valid(type t) noexcept
{
	return !details::is_check<type::MALFORMED>(t) && ((is_value(t) && !is_reference(t)) || is_object(t));
}

static bool is_range(type t) noexcept { return details::is_check<type::RANGE>(t); }
static bool is_value(type t) noexcept { return details::is_check<type::VALUE>(t); }
static bool is_object(type t) noexcept { return details::is_check<type::OBJECT>(t); }
static bool is_reference(type t) noexcept { return details::is_check<type::REFERENCE>(t); }

class node final
{
  public:
	node()	= default;
	~node() = default;

	node(const node& other)				   = default;
	node(node&& other) noexcept			   = default;
	node& operator=(const node& other)	   = default;
	node& operator=(node&& other) noexcept = default;

	void content(psl::string8::view value);
	void name(psl::string8::view value);
	bool parent(const node& value);

	psl::string8::view content() const noexcept;

  private:
	type m_Type;
	psl::string8_t* m_Name;
	void* m_Data {nullptr};
};
}	 // namespace psl::format
