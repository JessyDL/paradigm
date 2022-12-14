#pragma once
#include "psl/ustring.hpp"
#include <memory>
#include <numeric>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

/*
	string based version:
		{ optional header }
		MAJOR.MINOR.PATCH
	{ body }
		value_t:					[FLOAT]2.5[/FLOAT]
		value_t (literal):			[FLOAT]'''2.5'''[/FLOAT]
		value_t:					[SHADER]AF7D9C55-17C0-431A-BB4D-98A02F0FFB80[/SHADER]
		value_range_t:				[FLOATS]{1.4, 3.9}[/FLOATS] or [FLOATS]{'''1.4''', 3.9}[/FLOATS]
		value_range_t (literal):	[FLOATS]{'''1.4''', 3.9}[/FLOATS]
		reference_t:				[SHADER_NAME]&COLLECTION::CHILD_STRING[/SHADER_NAME]
		reference_range_t:			[REFERENCENODES]&{FLOAT, SHADER_NAME}[/REFERENCENODES]
		collection_t:				[COLLECTION][CHILD_STRING]I'm a child![/CHILD_STRING][/COLLECTION]

	compact based verion:
		HEADER:
		PCMPDATA ...

		Table of contents:
		node name index, node content index, node type..

		content:
		name
		content
	binary based version:
		HEADER:
		PBINDATA ...
		Table of contents:
		node name index, node content index, node type..

		content:
		name
		content

	internal data representation:
		value_t: psl::string8::view -> view into the container where the content is.
		value_range_t: vector<string_view> -> view into the container for each content element.
		reference_t: nodes_t index of the node in the container vector
		reference_range_t: vector<nodes_t> index range of all nodes it is referencing in the container vector
		collection_t: pair<nodes_t, nodes_t> start index and size of the nodes in the parent (works like a range<T> )
*/


namespace psl::format {
struct handle;

using children_t		= uint16_t;
using nodes_t			= uint32_t;
using value_t			= std::pair<bool, std::pair<size_t, size_t>>;
using value_range_t		= std::vector<value_t>;
using reference_t		= handle*;
using reference_range_t = std::vector<reference_t>;
using collection_t		= nodes_t;

enum class type_t : uint8_t {
	MALFORMED		= 0,
	VALUE			= 1,
	VALUE_RANGE		= 2,
	REFERENCE		= 3,
	REFERENCE_RANGE = 4,
	COLLECTION		= 5
};

namespace constants {
	static const psl::string8_t EMPTY_CHARACTERS = " \n\t\r ";
	static const psl::string8_t HEAD_OPEN		 = "[";
	static const psl::string8_t HEAD_CLOSE		 = "]";
	static const psl::string8_t TAIL_OPEN		 = HEAD_OPEN + "/";
	static const psl::string8_t TAIL_CLOSE		 = HEAD_CLOSE;

	static const psl::string8_t LITERAL_OPEN  = "'''";
	static const psl::string8_t LITERAL_CLOSE = LITERAL_OPEN;

	static const psl::string8_t RANGE_OPEN	  = "{";
	static const psl::string8_t RANGE_CLOSE	  = "}";
	static const psl::string8_t RANGE_DIVIDER = ",";

	static const psl::string8_t REFERENCE		  = "&";
	static const psl::string8_t REFERENCE_MISSING = "MISSING_REFERENCE";
	static const psl::string8_t NAMESPACE_DIVIDER = "::";
}	 // namespace constants
struct partial_view {
	std::unique_ptr<partial_view> before {nullptr};
	psl::string8::view view;
	std::unique_ptr<partial_view> after {nullptr};
};

struct container;

struct settings {
	bool pretty_write	= true;
	bool write_header	= true;
	bool binary_value	= false;
	bool compact_string = false;
};


struct data final {
	friend struct container;

	static_assert(sizeof(collection_t) <= sizeof(uint64_t) * 4, "collection node will not fit in the buffer");
	static_assert(sizeof(reference_t) <= sizeof(uint64_t) * 4, "reference node will not fit in the buffer");
	static_assert(sizeof(reference_range_t) <= sizeof(uint64_t) * 4, "reference range node will not fit in the buffer");
	static_assert(sizeof(value_t) <= sizeof(uint64_t) * 4, "value node will not fit in the buffer");
	static_assert(sizeof(value_range_t) <= sizeof(uint64_t) * 4, "value range node will not fit in the buffer");


  private:
	data(container* parent, nodes_t index);
	value_range_t& reinterpret_as_value_range() const;
	value_t& reinterpret_as_value() const;
	reference_range_t& reinterpret_as_reference_range() const;
	reference_t& reinterpret_as_reference() const;
	collection_t& reinterpret_as_collection() const;

  public:
	psl::string8_t to_string(const format::settings& settings) const;
	void to_string(const format::settings& settings, psl::string8_t& out) const;
	data(const data& other)			   = delete;
	data& operator=(const data& other) = delete;
	data(data&&) noexcept			   = default;
	data& operator=(data&&) noexcept   = default;

	psl::string8::view name() const;
	container* root() const;
	type_t type() const { return m_Type; };
	format::children_t depth() const { return m_Depth; };
	// rebase the node onto a new parent.
	bool parent(data& parent);

	// remove the parent, and rebase onto root
	void unparent();

	// returns the parent if any
	std::optional<data*> parent();

	std::optional<value_range_t> as_value_range() const;
	std::optional<value_t> as_value() const;
	std::optional<reference_t> as_reference() const;
	std::optional<reference_range_t> as_reference_range() const;
	std::optional<collection_t> as_collection() const;

	std::optional<std::pair<bool, psl::string8::view>> as_value_content() const;
	std::optional<std::vector<std::pair<bool, psl::string8::view>>> as_value_range_content() const;

	void* _data() { return &buffer; };

  protected:
	uint64_t buffer[4] {0, 0, 0, 0};
	std::pair<size_t, size_t> m_Name;	 // 16
	handle* m_Handle {nullptr};
	format::children_t m_Depth {0};				  // 2
	format::type_t m_Type {type_t::MALFORMED};	  // 1
};


struct handle final {
	friend struct data;
	friend struct container;
	handle(nodes_t index, container* container) : m_Index(index), m_Container(container) {};
	handle(const handle& other) : m_Index(other.m_Index), m_Container(other.m_Container), parent(other.parent) {};
	handle& operator=(const handle& other) = delete;
	data& get() const;
	data* operator->() const;
	bool exists() const { return m_Container != nullptr; }
	bool operator==(const handle& other) const {
		return m_Container == other.m_Container && m_Index == other.m_Index && parent == other.parent;
	}
	bool operator!=(const handle& other) const {
		return m_Container != other.m_Container || m_Index != other.m_Index || parent != other.parent;
	}

  private:
	container* m_Container;
	nodes_t m_Index;
	handle* parent {nullptr};
};

struct container {
	struct compact_header {
		struct entry {
			size_t name_start;	  // element is always the start offset in respects to the names string's begin
			size_t name_size;
			size_t depth;
			uint8_t type;
		};

		struct content_info {
			size_t count;
			std::vector<size_t>
			  offsets;	  // first element is always the start offset in respects to the content string's begin
		};


		psl::string8_t build(const container& container);
		bool try_decode(psl::string8::view source, format::container& target);

		static constexpr psl::string8::view string_identifier = "PCMPDATA";
		static constexpr psl::string8::view bin_identifier	  = "PBINDATA";
		std::vector<entry> entries;
		std::vector<content_info> content_header;
		psl::string8_t names;
		psl::string8_t content;
	};

  private:
	struct version_t {
		version_t() {};
		~version_t() = default;
		uint32_t major {0u};
		uint32_t minor {0u};
	};

	friend struct data;
	friend struct handle;

  public:
	enum class encoding_t : uint8_t {
		string = 0b01,
		// binary serializes as much as it can into a binary stream of data. it also uses the same setup like
		// compact
		binary = 0b10
	};
	struct features {
		features() {};
		~features() = default;
		version_t version {};
		bool recover_from_error {true};
		bool verify_checksum {false};
		encoding_t encoding {encoding_t::string};
	};

	container(features _features = features());
	container(psl::string8::view content);
	~container();

	handle& operator[](psl::string8::view view) const;	  // Find any with given name operation
	handle& operator[](nodes_t index);					  // Get at index

	nodes_t index_of(psl::string8::view name) const;
	nodes_t index_of(const data& data) const;
	nodes_t index_of(const data& data, psl::string8::view child) const;
	bool contains(psl::string8::view name) const;
	size_t size() const { return m_NodeData.size(); };

	handle& add_value(psl::string8::view name, psl::string8::view content);
	handle& add_value_range(psl::string8::view name, std::vector<psl::string8::view> content);
	handle& add_collection(psl::string8::view name);
	handle& add_reference(psl::string8::view name, data& referencing);
	handle& add_reference_range(psl::string8::view name, std::vector<std::reference_wrapper<data>> referencing);
	handle& add_value(data& parent, psl::string8::view name, psl::string8::view content);
	handle& add_value_range(data& parent, psl::string8::view name, std::vector<psl::string8::view> content);
	handle& add_collection(data& parent, psl::string8::view name);
	handle& add_reference(data& parent, psl::string8::view name, data& referencing);
	handle&
	add_reference_range(data& parent, psl::string8::view name, std::vector<std::reference_wrapper<data>> referencing);

	bool parent(data& new_parent, data& node);
	std::optional<data*> parent(const data& node);
	void unparent(data& node);

	handle& find(const data& parent, psl::string8::view name);
	handle& find(psl::string8::view name);

	handle& operator+=(std::pair<psl::string8::view, psl::string8::view> value);

	psl::string8_t fullname(const data& node) const;

	bool remove(data& data);
	void reserve(size_t count) { m_NodeData.reserve(count); }

	psl::string8_t to_string(std::optional<format::settings> settings = std::nullopt) const;

	void validate();

	std::vector<data>::iterator begin() noexcept { return m_NodeData.begin(); };
	std::vector<data>::const_iterator begin() const noexcept { return m_NodeData.begin(); };
	std::vector<data>::iterator end() noexcept { return m_NodeData.end(); };
	std::vector<data>::const_iterator end() const noexcept { return m_NodeData.end(); };

	std::vector<data>::const_iterator cbegin() const noexcept { return m_NodeData.cbegin(); };
	std::vector<data>::const_iterator cend() const noexcept { return m_NodeData.cend(); };

	container(const container&) = delete;
	container(container&&);
	container& operator=(const container&) = delete;
	container& operator=(container&&);

	void clear_settings() { m_Settings = std::nullopt; }
	void set_settings(settings settings) { m_Settings = settings; }
	bool operator==(const container& other) const {
		return size() == other.size() && m_InternalData == other.m_InternalData && m_Content == other.m_Content;
	}

	bool set_reference(format::data& source, format::data& target);

  private:
	data& internal_get(psl::string8::view view);

	std::pair<nodes_t, children_t> create(data& parent, psl::string8::view& name);
	std::pair<nodes_t, children_t> create(psl::string8::view& name);
	nodes_t add_node(nodes_t parent);
	nodes_t add_node();

	void move(data& data, nodes_t index);
	std::pair<size_t, size_t> insert_name(psl::string8::view name, nodes_t index);
	value_t insert_content(psl::string8::view content, nodes_t index);
	value_range_t insert_content(std::vector<psl::string8::view> content, nodes_t index);

	void parse(const psl::string8::view& file);

	size_t parse(const psl::string8::view& content,
				 std::vector<data>& node_data,
				 std::vector<nodes_t>& reference_nodes,
				 children_t depth = 0);
	void parse_references(container& container, std::vector<data>& data, const std::vector<nodes_t>& reference_nodes);

	psl::string8_t m_InternalData {};
	psl::string8_t m_Content {};
	std::vector<data> m_NodeData {};
	features m_Features {};
	std::optional<settings> m_Settings;
	format::handle m_TerminalHandle {0, nullptr};
};


struct node_not_found : public std::exception {
	node_not_found(container const* const container, nodes_t index) : m_Container(container), m_Data(index) {};
	node_not_found(container const* const container, psl::string8::view name)
		: m_Container(container), m_Data(name.data()) {};

	container const* const target() const noexcept { return m_Container; }

	char const* what() const noexcept override;

  public:
	container const* const m_Container;
	std::variant<nodes_t, psl::string8_t> m_Data;
	mutable psl::string8_t m_Message {};
};

struct duplicate_node : public std::exception {
	duplicate_node(psl::string8::view name) : name(name) {};
	char const* what() const noexcept override {
		message = "duplicate node found using the name: ";
		message += name;
		return message.data();
	}

  private:
	psl::string8::view name;
	mutable psl::string8_t message;
};

struct max_depth_reached : public std::exception {
	char const* what() const noexcept override {
		message = "The depth went over the: " + std::to_string(std::numeric_limits<children_t>::max()) + " limits";
		return message.data();
	}
	mutable psl::string8_t message;
};

struct max_nodes_reached : public std::exception {
	char const* what() const noexcept override {
		message = "The node count went over the: " + std::to_string(std::numeric_limits<nodes_t>::max()) + " limits";
		return message.data();
	}
	mutable psl::string8_t message;
};
}	 // namespace psl::format
