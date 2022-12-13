#include "psl/format.hpp"
#include <algorithm>

#if __has_include(<functional>)
	#include <functional>
#else
	#include <experimental/functional>
#endif
#include "psl/crc32.hpp"
#include "psl/string_utils.hpp"
#include <numeric>
#include <stack>
#include <unordered_map>
#ifdef PLATFORM_LINUX
	// todo: find cleaner solution to this, https://bugzilla.redhat.com/show_bug.cgi?id=130601 not a bug my ass, it's
	// like the windows min/max..
	#undef minor
	#undef major
#endif
using namespace psl::format;


psl::format::data::data(container* parent, nodes_t index) : m_Handle(new handle(index, parent)) {}

value_range_t& psl::format::data::reinterpret_as_value_range() const { return *((value_range_t*)(buffer)); }

value_t& psl::format::data::reinterpret_as_value() const { return *((value_t*)(&buffer)); }

reference_range_t& psl::format::data::reinterpret_as_reference_range() const { return *((reference_range_t*)(buffer)); }

reference_t& psl::format::data::reinterpret_as_reference() const { return *((reference_t*)(&buffer)); }

collection_t& psl::format::data::reinterpret_as_collection() const { return *((collection_t*)(&buffer)); }

std::optional<value_range_t> data::as_value_range() const
{
	if(m_Type != type_t::VALUE_RANGE) return {};
	return *((value_range_t*)(buffer));
}

std::optional<value_t> data::as_value() const
{
	if(m_Type != type_t::VALUE) return {};
	return *((value_t*)(&buffer));
}

std::optional<reference_t> data::as_reference() const
{
	if(m_Type != type_t::REFERENCE) return {};
	return *((reference_t*)(&buffer));
}

std::optional<reference_range_t> data::as_reference_range() const
{
	if(m_Type != type_t::REFERENCE_RANGE) return {};
	return *((reference_range_t*)(buffer));
}

std::optional<collection_t> data::as_collection() const
{
	if(m_Type != type_t::COLLECTION) return {};
	return *((collection_t*)(&buffer));
}


size_t search(const psl::string8::view::const_iterator& first,
			  const psl::string8::view::const_iterator& last,
			  const psl::string8::view& pattern)
{
	// auto res = std::search(first, last,
	// std::boyer_moore_searcher<psl::string8::view::const_iterator>(pattern.cbegin(), pattern.cend()));
#ifdef _MSC_VER
	auto res = std::search(
	  first, last, std::default_searcher<psl::string8::view::const_iterator>(pattern.cbegin(), pattern.cend()));
#else
	// todo: wait till later clang to support the <functional> header correctly
	auto res = std::search(first, last, pattern.cbegin(), pattern.cend());
#endif
	// auto res = std::search(first, last,
	// std::boyer_moore_horspool_searcher<psl::string8::view::const_iterator>(pattern.cbegin(), pattern.cend()));
	if(res == last) return psl::string8_t::npos;

	return res - first;
}

size_t search(const psl::string8::view& content, const psl::string8::view& pattern, size_t offset = 0)
{
	if(auto loc = search((content.begin() + offset), content.cend(), pattern); loc != psl::string8_t::npos)
		return loc + offset;
	return psl::string8_t::npos;
}

std::tuple<type_t, size_t> parse_type(const psl::string8::view& content, size_t node_content_begin_n)
{
	auto content_begin_n = content.find_first_not_of(constants::EMPTY_CHARACTERS, node_content_begin_n);
	return std::make_tuple(
	  (content.compare(content_begin_n, constants::HEAD_OPEN.size(), constants::HEAD_OPEN) == 0) ? type_t::COLLECTION
	  : (content.compare(content_begin_n, constants::HEAD_OPEN.size(), constants::RANGE_OPEN) == 0)
		? ((content.compare(content_begin_n + 1, constants::HEAD_OPEN.size(), constants::REFERENCE) == 0)
			 ? type_t::REFERENCE_RANGE
			 : type_t::VALUE_RANGE)
	  : (content.compare(content_begin_n, constants::HEAD_OPEN.size(), constants::REFERENCE) == 0) ? type_t::REFERENCE
																								   : type_t::VALUE,
	  content_begin_n);
}

bool is_literal(const psl::string8::view& content, size_t node_content_begin_non_empty_n)
{
	return content.compare(node_content_begin_non_empty_n, constants::LITERAL_OPEN.size(), constants::LITERAL_OPEN) ==
		   0;
}


psl::string8_t::size_type
rfind_first_not_of(psl::string8::view const& str, psl::string8_t::size_type const pos, psl::string8::view const& chars)
{
	if(chars.find(str[pos - 1]) != psl::string8_t::npos) return rfind_first_not_of(str, pos - 1, chars);
	return pos ? pos : psl::string8_t::npos;
}

psl::string8_t::size_type
rfind_first_of(psl::string8::view const& str, psl::string8_t::size_type const pos, psl::string8::view const& chars)
{
	if(pos > 0 && chars.find(str[pos - 1]) != psl::string8_t::npos) return pos;
	return (pos == 0) ? psl::string8_t::npos : rfind_first_of(str, pos - 1, chars);
}

bool psl::format::container::remove(data& data)
{
	if(data.root() != this) return false;
	auto index				   = index_of(data);
	size_t content_shift_value = 0;
	size_t content_shift_index = 0;
	nodes_t node_count		   = 1;
	switch(data.m_Type)
	{
	case type_t::VALUE:
	{
		content_shift_value = data.reinterpret_as_value().second.second;
		content_shift_index = data.reinterpret_as_value().second.first;
	}
	break;
	case type_t::VALUE_RANGE:
	{
		const auto& values = data.reinterpret_as_value_range();
		if(values.size() > 0) content_shift_index = values[0].second.first;
		for(const auto& it : values)
		{
			content_shift_value += it.second.second;
		}
	}
	break;
	case type_t::COLLECTION:
	{
		const auto& children_n = data.reinterpret_as_collection();
		nodes_t current		   = index;
		content_shift_index	   = 0;
		while(current <= index + children_n && content_shift_index == 0)
		{
			switch(m_NodeData[current].m_Type)
			{
			case type_t::VALUE:
			{
				const auto& val		= m_NodeData[current].reinterpret_as_value();
				content_shift_index = val.second.first;
			}
			break;
			case type_t::VALUE_RANGE:
			{
				const auto& values = m_NodeData[current].reinterpret_as_value_range();
				if(values.size() > 0)
				{
					content_shift_index = values[0].second.first;
				}
			}
			break;
			default:
			{
			};
			}
			current++;
		}

		node_count += children_n;
		content_shift_value = 0;
		current				= index + children_n;
		while(current > index && content_shift_value == 0)
		{
			switch(m_NodeData[current].m_Type)
			{
			case type_t::VALUE:
			{
				const auto& val		= m_NodeData[current].reinterpret_as_value();
				content_shift_value = val.second.first + val.second.second - content_shift_index;
			}
			break;
			case type_t::VALUE_RANGE:
			{
				const auto& values = m_NodeData[current].reinterpret_as_value_range();
				if(values.size() > 0)
				{
					content_shift_value = values[values.size() - 1].second.first +
										  values[values.size() - 1].second.second - content_shift_index;
				}
			}
			break;
			default:
			{
			};
			}
			--current;
		}
	}
	break;
	default:
	{
	};
	}

	if(content_shift_value > 0)
	{
		m_Content.erase(m_Content.begin() + content_shift_index,
						m_Content.begin() + content_shift_index + content_shift_value);
		for(auto i = index + node_count; i < m_NodeData.size(); ++i)
		{
			switch(m_NodeData[i].m_Type)
			{
			case type_t::VALUE:
			{
				auto& value = m_NodeData[i].reinterpret_as_value();
				value.second.first -= content_shift_value;
			}
			break;
			case type_t::VALUE_RANGE:
			{
				auto& values = *((value_range_t*)(m_NodeData[i].buffer));
				for(auto& it : values)
				{
					it.second.first -= content_shift_value;
				}
			}
			break;
			default:
			{
			};
			}
		}
	}

	size_t name_shift_value = data.m_Name.second;
	size_t name_shift_index = data.m_Name.first;
	if(data.m_Type == type_t::COLLECTION)
	{
		const auto& children_n = data.reinterpret_as_collection();
		name_shift_value =
		  m_NodeData[index + children_n].m_Name.first + m_NodeData[index + children_n].m_Name.second - name_shift_index;
	}

	m_InternalData.erase(m_InternalData.begin() + name_shift_index,
						 m_InternalData.begin() + name_shift_index + name_shift_value);

	for(auto i = index + node_count; i < m_NodeData.size(); ++i)
	{
		m_NodeData[i].m_Name.first -= name_shift_value;
	}

	auto* hierarchy = &data;
	while(hierarchy->m_Depth > 0)
	{
		auto _parent = parent(*hierarchy);
		_parent.value()->reinterpret_as_collection() -= node_count;
		hierarchy = _parent.value();
	}

	for(auto i = index + node_count + 1; i < m_NodeData.size(); ++i)
	{
		m_NodeData[i].m_Handle->m_Index -= node_count;
	}

	for(auto i = index; i < index + node_count; ++i)
	{
		delete(m_NodeData[i].m_Handle);
	}

	m_NodeData.erase(m_NodeData.begin() + index, m_NodeData.begin() + index + node_count);

	return true;
}

nodes_t psl::format::container::add_node(nodes_t parent)
{
	if(m_NodeData.size() + 1 == std::numeric_limits<nodes_t>::max()) throw new max_nodes_reached();
	auto& children_n	 = m_NodeData[parent].reinterpret_as_collection();
	nodes_t insert_index = parent + children_n + 1;
	children_n += 1;
	m_NodeData.emplace(m_NodeData.begin() + insert_index, std::forward<data>(data(this, insert_index)));
	m_NodeData[insert_index].m_Handle->parent = m_NodeData[parent].m_Handle;

	if(m_NodeData[parent].m_Depth + 1 == std::numeric_limits<children_t>::max())
	{
		throw new max_depth_reached();
	}

	m_NodeData[insert_index].m_Depth = m_NodeData[parent].m_Depth + 1;

	for(auto i = insert_index + 1; i < m_NodeData.size(); ++i)
	{
		m_NodeData[i].m_Handle->m_Index += 1;
	}

	auto* hierarchy = &m_NodeData[parent];
	while(hierarchy->m_Depth > 0)
	{
		auto _parent = this->parent(*hierarchy);
		_parent.value()->reinterpret_as_collection() += 1;
		hierarchy = _parent.value();
	}

	return insert_index;
}
nodes_t psl::format::container::add_node()
{
	if(m_NodeData.size() + 1 == std::numeric_limits<nodes_t>::max()) throw new max_nodes_reached();
	m_NodeData.emplace_back(std::forward<data>(data(this, (nodes_t)m_NodeData.size())));
	return (nodes_t)(m_NodeData.size() - 1u);
}

std::pair<size_t, size_t> psl::format::container::insert_name(psl::string8::view name, nodes_t index)
{
	size_t name_shift_value = name.size();
	size_t name_shift_index = 0;
	if(index > 0)
	{
		auto& previous	 = m_NodeData[index - 1];
		name_shift_index = previous.m_Name.first + previous.m_Name.second;
	}

	m_InternalData.insert(m_InternalData.begin() + name_shift_index, name.begin(), name.end());

	for(auto i = index + 1; i < m_NodeData.size(); ++i)
	{
		m_NodeData[i].m_Name.first += name_shift_value;
	}

	return std::make_pair(name_shift_index, name_shift_value);
}

value_t psl::format::container::insert_content(psl::string8::view content, nodes_t index)
{
	size_t content_shift_value = content.size();
	size_t content_shift_index = 0;
	auto rfind_n			   = index;
	while(rfind_n > 0 && content_shift_index == 0)
	{
		rfind_n -= 1;
		auto& previous = m_NodeData[rfind_n];
		switch(previous.m_Type)
		{
		case type_t::VALUE:
		{
			const auto& value	= previous.reinterpret_as_value();
			content_shift_index = value.second.first + value.second.second;
		}
		break;
		case type_t::VALUE_RANGE:
		{
			const auto& values = previous.reinterpret_as_value_range();
			if(values.size() > 0)
				content_shift_index = values[values.size() - 1].second.first + values[values.size() - 1].second.second;
		}
		break;
		default:
		{
		};
		}
	}

	m_Content.insert(m_Content.begin() + content_shift_index, content.begin(), content.end());


	for(auto i = index + 1; i < m_NodeData.size(); ++i)
	{
		switch(m_NodeData[i].m_Type)
		{
		case type_t::VALUE:
		{
			auto& value = m_NodeData[i].reinterpret_as_value();
			value.second.first += content_shift_value;
		}
		break;
		case type_t::VALUE_RANGE:
		{
			auto& values = m_NodeData[i].reinterpret_as_value_range();
			for(auto& it : values)
			{
				it.second.first += content_shift_value;
			}
		}
		break;
		default:
		{
		};
		}
	}

	return std::make_pair(false, std::make_pair(content_shift_index, content_shift_value));
}

value_range_t psl::format::container::insert_content(std::vector<psl::string8::view> content, nodes_t index)
{
	psl::string8_t agg_content;
	value_range_t res;
	res.reserve(content.size());
	agg_content.reserve(utility::string::size(content));
	for(const auto& it : content)
	{
		agg_content += it;
		res.emplace_back(std::make_pair(false, std::make_pair(agg_content.size() - it.size(), it.size())));
	}

	size_t content_shift_value = agg_content.size();
	size_t content_shift_index = 0;
	auto rfind_n			   = index;
	while(rfind_n > 0 && content_shift_index == 0)
	{
		rfind_n -= 1;
		auto& previous = m_NodeData[rfind_n];
		switch(previous.m_Type)
		{
		case type_t::VALUE:
		{
			const auto& value	= previous.reinterpret_as_value();
			content_shift_index = value.second.first + value.second.second;
		}
		break;
		case type_t::VALUE_RANGE:
		{
			const auto& values = previous.reinterpret_as_value_range();
			if(values.size() > 0)
				content_shift_index = values[values.size() - 1].second.first + values[values.size() - 1].second.second;
		}
		break;
		default:
		{
		};
		}
	}

	for(auto& it : res)
	{
		it.second.first += content_shift_index;
	}

	m_Content.reserve(std::max(m_Content.capacity(), m_Content.size() + content_shift_value));
	m_Content.insert(m_Content.begin() + content_shift_index, agg_content.begin(), agg_content.end());


	for(auto i = index + 1; i < m_NodeData.size(); ++i)
	{
		switch(m_NodeData[i].m_Type)
		{
		case type_t::VALUE:
		{
			auto& value = m_NodeData[i].reinterpret_as_value();
			value.second.first += content_shift_value;
		}
		break;
		case type_t::VALUE_RANGE:
		{
			auto& values = m_NodeData[i].reinterpret_as_value_range();
			for(auto& it : values)
			{
				it.second.first += content_shift_value;
			}
		}
		break;
		default:
		{
		};
		}
	}

	return res;
}

nodes_t psl::format::container::index_of(const data& data) const
{
	auto dif = std::uintptr_t(&data) - std::uintptr_t(m_NodeData.data());
	if(dif % sizeof(psl::format::data) != 0u || dif < 0u || dif > m_NodeData.size() * sizeof(psl::format::data))
	{
		throw new node_not_found(this, data.name());
	}

	return (nodes_t)(dif / sizeof(psl::format::data));
}

nodes_t psl::format::container::index_of(const data& data, psl::string8::view child) const
{
	nodes_t parent_index = index_of(data);

	std::vector<psl::string8::view> partNames = utility::string::split(child, constants::NAMESPACE_DIVIDER);
	children_t depth						  = m_NodeData[parent_index].m_Depth;
	auto children_n							  = m_NodeData[parent_index].reinterpret_as_collection();
	auto name								  = std::begin(partNames);
	for(auto i = parent_index + 1; i <= parent_index + children_n; ++i)
	{
		if(m_NodeData[i].m_Depth == (depth + 1) && m_NodeData[i].name() == *name)
		{
			++depth;
			name = std::next(name);
			if(name == std::end(partNames)) return i;
		}
		else if(m_NodeData[i].m_Type == type_t::COLLECTION)
		{
			i += m_NodeData[i].reinterpret_as_collection();
		}
	}

	return std::numeric_limits<nodes_t>::max();
}

psl::format::container::features parse_header(psl::string8::view content)
{
	if(content.size() == 0) return {};

	psl::string8_t boolean_values[] {"TRUE", "true", "T", "t", "True", "1", "yes", "YES", "Y", "y", "Yes"};
	psl::string8_t numeric_values {"0123456789"};
	psl::string8::view header {&content[0], search(content, constants::HEAD_OPEN)};

	psl::format::container::features features {};
	if(auto index = search(header, "CHECKSUM"); index != psl::string8_t::npos)
	{
		auto chksum_start = header.find_first_of(numeric_values, index);
		auto chksum_end	  = header.find_first_not_of(numeric_values, chksum_start);

		psl::string8::view headerless_content {&content[header.size()], content.size() - header.size()};
		auto checksum = utility::crc32(headerless_content);
		psl::string8_t checksum_file(&header[chksum_start], chksum_end - chksum_start);

		char* endptr = nullptr;
		if(features.verify_checksum)
		{
			auto calc_checksum = std::strtoul(checksum_file.data(), &endptr, 10);

			if(checksum != calc_checksum) throw new std::runtime_error("failed checksums");
		}
	}


	if(auto index = search(header, "VERSION"); index != psl::string8_t::npos)
	{
		index += sizeof("VERSION");
		{
			size_t digit_start = header.find_first_of(numeric_values, index);
			size_t digit_end   = header.find_first_not_of(numeric_values, digit_start);
			psl::string8_t major {&header[digit_start], digit_end - digit_start};
			char* endptr		   = nullptr;
			features.version.major = std::strtoul(major.data(), &endptr, 10);
			index				   = digit_end;
		}
		{
			auto digit_start = header.find_first_of(numeric_values, index);
			auto digit_end	 = header.find_first_not_of(numeric_values, digit_start);
			psl::string8_t minor(&header[digit_start], digit_end - digit_start);
			char* endptr		   = nullptr;
			features.version.minor = std::strtoul(minor.data(), &endptr, 10);
			index				   = digit_end;
		}
	}

	if(auto index = search(header, "FEATURE_RECOVER_ON_ERROR"); index != psl::string8_t::npos)
	{
		features.recover_from_error = false;
		index += sizeof("FEATURE_RECOVER_ON_ERROR");
		index = header.find_first_not_of(constants::EMPTY_CHARACTERS, index);
		for(const auto& it : boolean_values)
		{
			if(header.compare(index, it.size(), it))
			{
				features.recover_from_error = true;
				break;
			}
		}
	}


	return features;
}

container::container(features features) : m_Features(features) {}

container::container(psl::string8::view content) { parse(content); }


void container::parse(const psl::string8::view& file)
{
	m_InternalData.reserve(file.size() / 4);
	m_Content.reserve(file.size() / 2);
	m_InternalData.clear();
	m_Content.clear();

	compact_header header;
	if(!header.try_decode(file, *this))
	{
		m_Features = parse_header(file);
		std::vector<nodes_t> reference_nodes;
		parse(file, m_NodeData, reference_nodes);
		parse_references(*this, m_NodeData, reference_nodes);
	}
}

size_t container::parse(const psl::string8::view& content,
						std::vector<data>& node_data,
						std::vector<nodes_t>& reference_nodes,
						children_t depth)
{
	struct collection_view
	{
		collection_view(size_t content_begin_n, nodes_t index) : content_begin_n(content_begin_n), index(index) {}
		size_t content_begin_n;
		nodes_t index;
	};
	size_t node_begin_n = search(content, constants::HEAD_OPEN);

	std::stack<collection_view> collection_stack;

	if(node_begin_n == search(content, constants::TAIL_OPEN))
	{
		return node_begin_n;
	}

	std::stack<handle*> parent_stack;
	while(node_begin_n != psl::string8_t::npos)
	{
		auto index = node_data.size();

		if(node_data.size() + 1 == std::numeric_limits<nodes_t>::max()) throw new max_nodes_reached();

		node_data.emplace_back(std::forward<data>(data(this, (nodes_t)index)));
		if(parent_stack.size() > 0)
		{
			node_data[node_data.size() - 1].m_Handle->parent = parent_stack.top();
		}
		auto node_name_end_n = search(content, constants::HEAD_CLOSE, node_begin_n + 1);
		{
			auto name_size_n = node_name_end_n - (node_begin_n + 1);
			m_InternalData.append(&content[node_begin_n + 1], name_size_n);
			node_data[index].m_Name = std::make_pair(m_InternalData.size() - name_size_n, name_size_n);
			psl::string8::view name = node_data[index].name();

			if(name == "")	  // todo
			{
				throw new duplicate_node(name);
			}
		}

		auto node_content_end_n = psl::string8_t::npos;
		auto content_size_n		= psl::string8_t::npos;

		auto [type, content_begin_n] = parse_type(content, node_name_end_n + 1);
		node_data[index].m_Type		 = type;
		node_data[index].m_Depth	 = depth;

		psl::string8_t tail_name =
		  constants::TAIL_OPEN + psl::string8_t(node_data[index].name()) + constants::TAIL_CLOSE;
		switch(node_data[index].m_Type)
		{
		case type_t::COLLECTION:
		{
			++depth;
			if(depth == std::numeric_limits<children_t>::max())
			{
				throw new max_depth_reached();
			}
			parent_stack.push(node_data[node_data.size() - 1].m_Handle);
			collection_stack.push(collection_view(content_begin_n, (nodes_t)index));
		}
		break;
		case type_t::VALUE:
		{
			bool literal_value = is_literal(content, content_begin_n);
			content_begin_n += (literal_value) ? constants::LITERAL_OPEN.size() : 0;
			auto content_end_n =
			  search(content, (literal_value) ? constants::LITERAL_CLOSE : tail_name, content_begin_n);
			content_size_n	   = content_end_n - content_begin_n;
			node_content_end_n = (literal_value) ? content_end_n + constants::LITERAL_CLOSE.size() : content_end_n;
			m_Content.append(&content[content_begin_n], content_size_n);
			node_data[index].reinterpret_as_value() =
			  value_t {literal_value, std::make_pair(m_Content.size() - content_size_n, content_size_n)};
		}
		break;
		case type_t::VALUE_RANGE:
		{
			auto offset_n = content_begin_n;
			while(node_content_end_n == psl::string8_t::npos && offset_n != psl::string8_t::npos)
			{
				auto found_n = search(content, constants::RANGE_CLOSE, offset_n);
				found_n =
				  content.find_first_not_of(constants::EMPTY_CHARACTERS, found_n + constants::RANGE_CLOSE.size());
				if(content.compare(found_n, tail_name.size(), tail_name) == 0)
				{
					// todo: this might create an issue where the literals are ignored, watch out.
					node_content_end_n = found_n;
				}
			}

			offset_n =
			  content.find_first_not_of(constants::EMPTY_CHARACTERS, content_begin_n) + constants::RANGE_OPEN.size();
			bool bEnd = false;

			value_range_t* content_views = new(node_data[index]._data()) value_range_t();
			while(!bEnd)
			{
				auto sub_content_begin_n = content.find_first_not_of(constants::EMPTY_CHARACTERS, offset_n);
				auto sub_content_end_n	 = psl::string8_t::npos;
				bool literal			 = is_literal(content, sub_content_begin_n);
				auto next_offset_n		 = psl::string8_t::npos;
				if(literal)
				{
					sub_content_begin_n += constants::LITERAL_OPEN.size();
					auto literal_end_n = search(content, constants::LITERAL_CLOSE, sub_content_begin_n);
					next_offset_n	   = search(content, constants::RANGE_DIVIDER, literal_end_n);
				}
				else
				{
					next_offset_n = search(content, constants::RANGE_DIVIDER, offset_n);
				}
				if(next_offset_n == psl::string8_t::npos || next_offset_n >= node_content_end_n)
				{
					next_offset_n = content.rfind(constants::RANGE_CLOSE, node_content_end_n);
					bEnd		  = true;
				}

				sub_content_end_n		= content.find_last_not_of(constants::EMPTY_CHARACTERS, next_offset_n);
				auto sub_content_size_n = sub_content_end_n - sub_content_begin_n;
				if(sub_content_size_n > 0)
				{
					m_Content.append(&content[sub_content_begin_n], sub_content_size_n);
					content_views->emplace_back(
					  literal, std::make_pair(m_Content.size() - sub_content_size_n, sub_content_size_n));
				}
				offset_n = next_offset_n + constants::RANGE_DIVIDER.size();
			}
		}
		break;
		case type_t::REFERENCE:
		{
			reference_nodes.emplace_back((nodes_t)index);
			content_begin_n =
			  content.find_first_not_of(constants::EMPTY_CHARACTERS + constants::REFERENCE, content_begin_n);
			node_content_end_n = search(content, tail_name, content_begin_n);
			node_content_end_n = rfind_first_not_of(content, node_content_end_n, constants::EMPTY_CHARACTERS);
			psl::string8::view ref(&content[content_begin_n], node_content_end_n - content_begin_n);
			memcpy(node_data[index]._data(), &ref, sizeof(psl::string8::view));
		}
		break;
		case type_t::REFERENCE_RANGE:
		{
			reference_nodes.emplace_back((nodes_t)index);
			auto offset_n = content_begin_n;
			while(node_content_end_n == psl::string8_t::npos && offset_n != psl::string8_t::npos)
			{
				auto found_n = search(content, constants::RANGE_CLOSE, offset_n);
				found_n =
				  content.find_first_not_of(constants::EMPTY_CHARACTERS, found_n + constants::RANGE_CLOSE.size());
				if(content.compare(found_n, tail_name.size(), tail_name) == 0)
				{
					node_content_end_n = found_n;
				}
			}

			offset_n  = search(content, constants::REFERENCE, content_begin_n) + constants::REFERENCE.size();
			bool bEnd = false;

			std::vector<psl::string8::view>* content_views =
			  new(node_data[index]._data()) std::vector<psl::string8::view>();
			while(!bEnd)
			{
				auto sub_content_begin_n =
				  content.find_first_not_of(constants::EMPTY_CHARACTERS + constants::REFERENCE, offset_n);
				auto sub_content_end_n = psl::string8_t::npos;
				auto next_offset_n	   = search(content, constants::RANGE_DIVIDER, offset_n);

				if(next_offset_n == psl::string8_t::npos || next_offset_n >= node_content_end_n)
				{
					next_offset_n = content.rfind(constants::RANGE_CLOSE, node_content_end_n);
					bEnd		  = true;
				}

				sub_content_end_n		= content.find_last_not_of(constants::EMPTY_CHARACTERS, next_offset_n);
				auto sub_content_size_n = sub_content_end_n - sub_content_begin_n;

				if(sub_content_size_n > 0)
					content_views->emplace_back(&content[sub_content_begin_n], sub_content_size_n);

				offset_n = next_offset_n + constants::RANGE_DIVIDER.size();
			}
		}
		break;
		default:
			throw new std::runtime_error("invalid node detected");
		}
		size_t node_end_n = 0u;
		if(node_data[index].m_Type == type_t::COLLECTION)
		{
			node_end_n = content_begin_n;
		}
		else
		{
			node_end_n = search(content, tail_name, node_content_end_n) + tail_name.size();
		}
		node_begin_n = search(content, constants::HEAD_OPEN, node_end_n);

		auto next_tail_n = search(content, constants::TAIL_OPEN, node_end_n);
		// we found the end of a collection
		while(next_tail_n <= node_begin_n && next_tail_n != psl::string8_t::npos)
		{
			parent_stack.pop();
			auto view = collection_stack.top();
			node_data[view.index].reinterpret_as_collection() =
			  collection_t((nodes_t)(node_data.size() - view.index - 1));
			collection_stack.pop();
			--depth;

			node_end_n	 = search(content, constants::TAIL_CLOSE, next_tail_n) + constants::TAIL_CLOSE.size();
			node_begin_n = search(content, constants::HEAD_OPEN, node_end_n);
			next_tail_n	 = search(content, constants::TAIL_OPEN, node_end_n);
		}
	}
	m_Content.shrink_to_fit();
	m_InternalData.shrink_to_fit();
	return content.size();
}

void container::parse_references(container& container,
								 std::vector<data>& data,
								 const std::vector<nodes_t>& reference_nodes)
{
	for(const auto& it : reference_nodes)
	{
		if(data[it].m_Type == type_t::REFERENCE)
		{
			psl::string8::view node_view;
			memcpy(&node_view, data[it]._data(), sizeof(psl::string8::view));
			;
			data[it].reinterpret_as_reference() = internal_get(node_view).m_Handle;
		}
		else
		{
			std::vector<psl::string8::view> names = *(std::vector<psl::string8::view>*)(data[it]._data());
			((std::vector<psl::string8::view>*)(data[it]._data()))->~vector();
			reference_range_t* references = new(data[it]._data()) reference_range_t();
			references->resize(names.size());
			for(int u = 0; u < names.size(); ++u)
			{
				(*references)[u] = internal_get(names[u]).m_Handle;
			}
		}
	}
}

container::~container()
{
	for(int i = 0; i < m_NodeData.size(); ++i)
	{
		delete(m_NodeData[i].m_Handle);
		if(m_NodeData[i].m_Type == type_t::REFERENCE_RANGE)
		{
			m_NodeData[i].reinterpret_as_reference_range().~vector();
		}
		else if(m_NodeData[i].m_Type == type_t::VALUE_RANGE)
		{
			m_NodeData[i].reinterpret_as_value_range().~vector();
		}
	}
}

std::optional<data*> psl::format::data::parent() { return root()->parent(*this); }

std::optional<data*> container::parent(const data& node)
{
	if(node.m_Depth == 0) return {};
	return &node.m_Handle->parent->get();
}

void container::unparent(data& node)
{
	// todo
	throw new std::runtime_error("not implemented");
	// move(node, m_NodeData.size() - 1);
}


std::optional<std::pair<bool, psl::string8::view>> data::as_value_content() const
{
	if(m_Type != type_t::VALUE) return {};
	value_t* val = ((value_t*)(&buffer));
	;
	return std::make_pair(val->first, psl::string8::view {&root()->m_Content[val->second.first], val->second.second});
}


std::optional<std::vector<std::pair<bool, psl::string8::view>>> data::as_value_range_content() const
{
	if(m_Type != type_t::VALUE_RANGE) return {};
	value_range_t* val = ((value_range_t*)(buffer));
	std::vector<std::pair<bool, psl::string8::view>> res;
	res.reserve(val->size());
	for(auto& it : *val)
	{
		res.emplace_back(it.first, psl::string8::view {&root()->m_Content[it.second.first], it.second.second});
	}
	return res;
}

bool psl::format::container::set_reference(psl::format::data& source, psl::format::data& target)
{
	if(source.m_Type != psl::format::type_t::REFERENCE) return false;

	reference_t* res = (reference_t*)(source._data());
	*res			 = target.m_Handle;
	return true;
}

data& container::internal_get(psl::string8::view view) { return m_NodeData[index_of(view)]; }

handle& container::operator[](nodes_t index)
{
	if((size_t)index >= m_NodeData.size()) throw new node_not_found(this, index);
	return *m_NodeData[(size_t)index].m_Handle;
}

handle& container::operator[](psl::string8::view view) const { return *m_NodeData[index_of(view)].m_Handle; }
nodes_t container::index_of(psl::string8::view name) const
{
	std::vector<psl::string8::view> partNames = utility::string::split(name, constants::NAMESPACE_DIVIDER);
	children_t depth						  = 0;
	for(int i = 0; i < m_NodeData.size(); ++i)
	{
		if(m_NodeData[i].m_Depth == depth && m_NodeData[i].name() == partNames[depth])
		{
			++depth;
			if(depth == partNames.size()) return i;
		}
		else if(m_NodeData[i].m_Type == type_t::COLLECTION)
		{
			i += m_NodeData[i].reinterpret_as_collection();
		}
	}

	return std::numeric_limits<nodes_t>::max();
}

bool psl::format::data::parent(data& parent) { return root()->parent(parent, *this); }

void psl::format::data::unparent() { root()->unparent(*this); }

psl::string8_t data::to_string(const psl::format::settings& settings) const
{
	psl::string8_t res;
	to_string(settings, res);
	return res;
}


void data::to_string(const psl::format::settings& settings, psl::string8_t& out) const
{
	const char endl = '\n';
	const char tab	= '\t';
	psl::string8_t node_name {name()};
	out += psl::string8_t(m_Depth * (size_t)(settings.pretty_write), tab) + constants::HEAD_OPEN + node_name +
		   constants::HEAD_CLOSE;
	switch(m_Type)
	{
	case type_t::COLLECTION:
	{
		if(settings.pretty_write)
		{
			auto children_n = reinterpret_as_collection();
			out += endl;
			auto my_index = root()->index_of(*this);
			for(auto i = my_index + 1; i <= my_index + children_n; ++i)
			{
				const auto& node = (*root()).m_NodeData[i];
				node.to_string(settings, out);
				out += endl;
				if(node.m_Type == type_t::COLLECTION)
				{
					i += node.reinterpret_as_collection();
				}
			}
			out += psl::string8_t(m_Depth, tab);
		}
		else
		{
			auto children_n = reinterpret_as_collection();
			auto my_index	= root()->index_of(*this);
			for(nodes_t i = 1u; i <= children_n; ++i)
			{
				const auto& node = (*root()).m_NodeData[i + my_index];
				node.to_string(settings, out);
				if(node.m_Type == type_t::COLLECTION)
				{
					i += node.reinterpret_as_collection();
				}
			}
		}
	}
	break;
	case type_t::VALUE:
	{
		auto val = as_value_content().value();
		out += (val.first) ? constants::LITERAL_OPEN : "";
		out += val.second;
		out += (val.first) ? constants::LITERAL_CLOSE : "";
	}
	break;
	case type_t::REFERENCE:
	{
		auto val = as_reference().value();
		if(val->m_Index == std::numeric_limits<nodes_t>().max())
			out += constants::REFERENCE + constants::REFERENCE_MISSING;
		else
			out += constants::REFERENCE + root()->fullname(val->get());
	}
	break;
	case type_t::VALUE_RANGE:
	{
		auto val = as_value_range_content().value();
		out += constants::RANGE_OPEN;
		for(const auto& it : val)
		{
			out += (it.first) ? constants::LITERAL_OPEN : "";
			out += it.second;
			out += (it.first) ? constants::LITERAL_CLOSE : "";
			out += constants::RANGE_DIVIDER;
		}
		if(val.size() > 0) out.erase(out.size() - 1);
		out += constants::RANGE_CLOSE;
	}
	break;
	case type_t::REFERENCE_RANGE:
	{
		out += constants::RANGE_OPEN;
		auto val = as_reference_range().value();
		for(auto& it : val)
		{
			if(((it))->m_Index == std::numeric_limits<nodes_t>().max())
				out += constants::REFERENCE + constants::REFERENCE_MISSING + constants::RANGE_DIVIDER;
			else
				out += constants::REFERENCE + root()->fullname(it->get()) + constants::RANGE_DIVIDER;
		}
		if(val.size() > 0) out.erase(out.size() - constants::RANGE_DIVIDER.size());
		out += constants::RANGE_CLOSE;
	}
	break;
	default:
	{
		std::runtime_error("unknown node type");
	};
	}

	out += constants::TAIL_OPEN + node_name + constants::TAIL_CLOSE;
}

psl::string8::view data::name() const
{
	return psl::string8::view {&(root()->m_InternalData[m_Name.first]), m_Name.second};
}

container* psl::format::data::root() const { return m_Handle->m_Container; }

handle& container::add_value(psl::string8::view name, psl::string8::view content)
{
	auto [node_index, depth]	   = create(name);
	m_NodeData[node_index].m_Type  = type_t::VALUE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;

	value_t* res = new(m_NodeData[node_index]._data()) value_t();
	*res		 = insert_content(content, node_index);
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_value(data& parent, psl::string8::view name, psl::string8::view content)
{
	auto [node_index, depth]	   = create(parent, name);
	m_NodeData[node_index].m_Type  = type_t::VALUE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;

	value_t* res = new(m_NodeData[node_index]._data()) value_t();
	*res		 = insert_content(content, node_index);
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_value_range(psl::string8::view name, std::vector<psl::string8::view> content)
{
	auto [node_index, depth]	   = create(name);
	m_NodeData[node_index].m_Type  = type_t::VALUE_RANGE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;

	value_range_t* res = new(m_NodeData[node_index]._data()) value_range_t();
	*res			   = std::move(insert_content(content, node_index));
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_value_range(data& parent, psl::string8::view name, std::vector<psl::string8::view> content)
{
	auto [node_index, depth]	   = create(parent, name);
	m_NodeData[node_index].m_Type  = type_t::VALUE_RANGE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;

	value_range_t* res = new(m_NodeData[node_index]._data()) value_range_t();
	*res			   = std::move(insert_content(content, node_index));
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_collection(psl::string8::view name)
{
	auto [node_index, depth] = create(name);

	m_NodeData[node_index].m_Type  = type_t::COLLECTION;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;
	collection_t* children_n	   = new(m_NodeData[node_index]._data()) collection_t();
	children_n					   = 0;
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_collection(data& parent, psl::string8::view name)
{
	auto [node_index, depth] = create(parent, name);

	m_NodeData[node_index].m_Type  = type_t::COLLECTION;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;
	collection_t* children_n	   = new(m_NodeData[node_index]._data()) collection_t();
	children_n					   = 0;
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_reference(psl::string8::view name, data& referencing)
{
	reference_t ref_index	 = referencing.m_Handle;
	auto [node_index, depth] = create(name);

	m_NodeData[node_index].m_Type  = type_t::REFERENCE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;
	reference_t* res			   = new(m_NodeData[node_index]._data()) reference_t();
	*res						   = ref_index;
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_reference(data& parent, psl::string8::view name, data& referencing)
{
	reference_t ref_index	 = referencing.m_Handle;
	auto [node_index, depth] = create(parent, name);

	m_NodeData[node_index].m_Type  = type_t::REFERENCE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;
	reference_t* res			   = new(m_NodeData[node_index]._data()) reference_t();
	*res						   = ref_index;
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_reference_range(psl::string8::view name, std::vector<std::reference_wrapper<data>> referencing)
{
	reference_range_t ref_indices;
	ref_indices.reserve(referencing.size());
	for(auto i = 0u; i < referencing.size(); ++i) ref_indices.emplace_back(referencing[i].get().m_Handle);
	auto [node_index, depth] = create(name);

	m_NodeData[node_index].m_Type  = type_t::REFERENCE_RANGE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;
	reference_range_t* res		   = new(m_NodeData[node_index]._data()) reference_range_t();
	*res						   = ref_indices;
	return *m_NodeData[node_index].m_Handle;
}

handle& container::add_reference_range(data& parent,
									   psl::string8::view name,
									   std::vector<std::reference_wrapper<data>> referencing)
{
	reference_range_t ref_indices;
	ref_indices.reserve(referencing.size());
	for(auto i = 0u; i < referencing.size(); ++i) ref_indices.emplace_back(referencing[i].get().m_Handle);
	auto [node_index, depth] = create(parent, name);

	m_NodeData[node_index].m_Type  = type_t::REFERENCE_RANGE;
	m_NodeData[node_index].m_Name  = insert_name(name, node_index);
	m_NodeData[node_index].m_Depth = depth;
	reference_range_t* res		   = new(m_NodeData[node_index]._data()) reference_range_t();
	*res						   = ref_indices;
	return *m_NodeData[node_index].m_Handle;
}


bool psl::format::container::parent(data& new_parent, data& node)
{
	if(node.root() != this) return false;

	// auto index_node = index_of(node);
	// auto index_parent = index_of(new_parent);

	return false;
}
std::pair<nodes_t, children_t> container::create(data& parent, psl::string8::view& name)
{
	nodes_t node_index = (nodes_t)m_NodeData.size();	// seeing we are adding one afterwards, this value is correct
														// after the upcoming if statement
	children_t depth  = 0;
	auto parent_index = index_of(parent);
	if(auto index = rfind_first_of(name, name.size(), constants::NAMESPACE_DIVIDER); index != psl::string8_t::npos)
	{
		psl::string8_t parent_name =
		  fullname(parent) + psl::string8_t {name.substr(0, index - constants::NAMESPACE_DIVIDER.size())};
		parent_index = index_of(parent_name);
		name		 = name.substr(index);
	}

	node_index = add_node(parent_index);
	depth	   = m_NodeData[parent_index].m_Depth + 1;
	return std::make_pair(node_index, depth);
}

std::pair<nodes_t, children_t> container::create(psl::string8::view& name)
{
	nodes_t node_index = (nodes_t)m_NodeData.size();	// seeing we are adding one afterwards, this value is correct
														// after the upcoming if statement
	children_t depth = 0;
	if(auto index = rfind_first_of(name, name.size(), constants::NAMESPACE_DIVIDER); index != psl::string8_t::npos)
	{
		auto parent_index = index_of(psl::string8::view(&name[0], index - constants::NAMESPACE_DIVIDER.size()));
		name			  = name.substr(index);
		node_index		  = add_node(parent_index);
		depth			  = m_NodeData[parent_index].m_Depth + 1;
	}
	else
	{
		add_node();
	}
	return std::make_pair(node_index, depth);
}

handle& container::operator+=(std::pair<psl::string8::view, psl::string8::view> value)
{
	return add_value(value.first, value.second);
}


psl::string8_t container::compact_header::build(const container& container)
{
	names	= container.m_InternalData;
	content = container.m_Content;
	for(int i = 0; i < container.m_NodeData.size(); ++i)
	{
		compact_header::entry& entry	   = entries.emplace_back();
		entry.type						   = (uint8_t)container.m_NodeData[i].m_Type;
		entry.name_start				   = container.m_NodeData[i].m_Name.first;
		entry.name_size					   = container.m_NodeData[i].m_Name.second;
		entry.depth						   = container.m_NodeData[i].m_Depth;
		compact_header::content_info& info = content_header.emplace_back();

		switch(container.m_NodeData[i].m_Type)
		{
		case type_t::COLLECTION:
		{
			info.count = 1;
			auto val   = container.m_NodeData[i].as_collection().value();
			info.offsets.emplace_back(val);
		}
		break;
		case type_t::VALUE:
		{
			info.count = 2;
			auto val   = container.m_NodeData[i].as_value().value();
			info.offsets.emplace_back(val.second.first);
			info.offsets.emplace_back(val.second.second);
		}
		break;
		case type_t::REFERENCE:
		{
			info.count = 1;
			auto val   = container.m_NodeData[i].as_reference().value();
			info.offsets.emplace_back(val->m_Index);
		}
		break;
		case type_t::VALUE_RANGE:
		{
			auto val = container.m_NodeData[i].as_value_range().value();
			if(val.size() == 0)
			{
				info.count = 0;
				continue;
			}
			info.count = val.size() + 1;
			info.offsets.emplace_back(val[0].second.first);
			for(const auto& v : val)
			{
				info.offsets.emplace_back(v.second.second);
			}
		}
		break;
		case type_t::REFERENCE_RANGE:
		{
			auto val = container.m_NodeData[i].as_reference_range().value();
			if(val.size() == 0)
			{
				info.count = 0;
				continue;
			}
			info.count = val.size();
			for(const auto& v : val)
			{
				info.offsets.emplace_back(v->m_Index);
			}
		}
		break;
		default:
		{
			throw std::runtime_error("unknown node type");
		};
		}
	}

	psl::string8_t res;
	size_t entry_size			   = entries.size();
	size_t content_info_size	   = content_header.size();
	size_t content_info_size_bytes = 0u;
	for(const auto& cHeader : content_header)
	{
		content_info_size_bytes += sizeof(size_t) * (cHeader.offsets.size() + 1);
	}

	res.resize(sizeof(psl::string8::char_t) * 8 + sizeof(size_t) * 2 + sizeof(entry) * entries.size() +
			   content_info_size_bytes + sizeof(psl::string8::char_t) * names.size() +
			   sizeof(psl::string8::char_t) * content.size());


	char* data = res.data();
	memcpy(data,
		   (container.m_Features.encoding == encoding_t::string) ? string_identifier.data() : bin_identifier.data(),
		   sizeof(psl::string8::char_t) * 8);
	data += sizeof(psl::string8::char_t) * 8;
	memcpy(data, &entry_size, sizeof(size_t));
	data += sizeof(size_t);
	memcpy(data, &content_info_size, sizeof(size_t));
	data += sizeof(size_t);
	memcpy(data, entries.data(), sizeof(entry) * entries.size());
	data += sizeof(entry) * entries.size();
	for(const auto& cHeader : content_header)
	{
		size_t count = cHeader.count;
		memcpy(data, &count, sizeof(size_t));
		data += sizeof(size_t);
		memcpy(data, cHeader.offsets.data(), sizeof(size_t) * cHeader.offsets.size());
		data += sizeof(size_t) * cHeader.offsets.size();
	}
	memcpy(data, names.data(), sizeof(psl::string8::char_t) * names.size());
	data += sizeof(psl::string8::char_t) * names.size();
	memcpy(data, content.data(), sizeof(psl::string8::char_t) * content.size());
	return res;
}

bool container::compact_header::try_decode(psl::string8::view source, psl::format::container& target)
{
	if(source.size() < 10)
	{
		return false;
	}
	psl::string8::view id {source.data(), 8};
	if(id != string_identifier && id != bin_identifier) return false;

	target.m_Features.encoding = (id == string_identifier) ? encoding_t::string : encoding_t::binary;

	size_t entry_size {0};
	size_t content_header_size {0};
	const char* data = source.data() + sizeof(psl::string8::char_t) * 8;
	memcpy(&entry_size, data, sizeof(size_t));
	data += sizeof(size_t);
	memcpy(&content_header_size, data, sizeof(size_t));
	data += sizeof(size_t);

	entries.resize(entry_size);
	memcpy(entries.data(), data, sizeof(entry) * entries.size());
	data += sizeof(entry) * entries.size();
	content_header.resize(content_header_size);
	target.m_NodeData.reserve(entries.size());
	size_t content_end_value = 0u;
	for(size_t i = 0; i < entries.size(); ++i)
	{
		memcpy(&content_header[i].count, data, sizeof(size_t));
		data += sizeof(size_t);
		content_header[i].offsets.resize(content_header[i].count);
		memcpy(content_header[i].offsets.data(), data, sizeof(size_t) * content_header[i].offsets.size());
		data += sizeof(size_t) * content_header[i].offsets.size();
		auto& node =
		  target.m_NodeData.emplace_back(std::forward<psl::format::data>(psl::format::data(&target, (nodes_t)i)));
		node.m_Type	 = (psl::format::type_t)entries[i].type;
		node.m_Name	 = std::make_pair(entries[i].name_start, entries[i].name_size);
		node.m_Depth = (children_t)entries[i].depth;
		switch(node.type())
		{
		case type_t::COLLECTION:
		{
			auto& val = node.reinterpret_as_collection();
			val		  = (collection_t)content_header[i].offsets[0];
		}
		break;
		case type_t::VALUE:
		{
			auto& val		  = node.reinterpret_as_value();
			val.second.first  = content_header[i].offsets[0];
			val.second.second = content_header[i].offsets[1];
			content_end_value = val.second.first + val.second.second;
		}
		break;
		case type_t::VALUE_RANGE:
		{
			auto& val = node.reinterpret_as_value_range();
			if(content_header[i].count == 0) continue;
			val.resize(content_header[i].count - 1);

			size_t offset = content_header[i].offsets[0];
			for(size_t v = 0; v < content_header[i].count - 1; ++v)
			{
				val[v].second.first	 = offset;
				val[v].second.second = content_header[i].offsets[v + 1];
				offset += content_header[i].offsets[v + 1];
				content_end_value = val[v].second.first + val[v].second.second;
			}
		}
		break;
		default:
		{
		};
		}
	}

	for(size_t i = 0; i < entries.size(); ++i)
	{
		auto& node = target.m_NodeData[i];
		switch(node.type())
		{
		case type_t::REFERENCE:
		{
			size_t index = content_header[i].offsets[0];
			auto& val	 = node.reinterpret_as_reference();
			val			 = target.m_NodeData[index].m_Handle;
		}
		break;
		case type_t::REFERENCE_RANGE:
		{
			auto& val = node.reinterpret_as_reference_range();
			if(content_header[i].count == 0) continue;

			for(size_t v = 0; v < content_header[i].count; ++v)
			{
				val[v] = target.m_NodeData[content_header[i].offsets[v]].m_Handle;
			}
		}
		break;
		default:
		{
		};
		}
	}

	size_t names_end = entries[entries.size() - 1].name_start + entries[entries.size() - 1].name_size;
	target.m_InternalData.resize(names_end);
	memcpy(target.m_InternalData.data(), data, sizeof(psl::string8::char_t) * target.m_InternalData.size());
	data += sizeof(psl::string8::char_t) * target.m_InternalData.size();
	target.m_Content.resize(content_end_value);
	memcpy(target.m_Content.data(), data, sizeof(psl::string8::char_t) * target.m_Content.size());
	return true;
}
psl::string8_t container::to_string(std::optional<psl::format::settings> settings) const
{
	psl::string8_t res;
	if(!settings && m_Settings) settings = m_Settings;
	psl::format::settings setting {};
	if(settings) setting = settings.value();

	if(setting.compact_string)
	{
		// size_t reserve = m_Content.size() + m_InternalData.size();
		compact_header header;

		return header.build(*this);
	}
	else
	{
		size_t reserve = m_Content.size() + m_InternalData.size() + m_InternalData.size() + res.size() +
						 (m_Content.size() + m_InternalData.size()) / 10u;
		for(int i = 0; i < m_NodeData.size(); ++i)
		{
			reserve += m_NodeData[i].m_Depth * (size_t)(setting.pretty_write) + constants::HEAD_OPEN.size() +
					   constants::HEAD_CLOSE.size() + constants::TAIL_OPEN.size() + constants::TAIL_CLOSE.size();

			switch(m_NodeData[i].m_Type)
			{
			case type_t::COLLECTION:
			{
				if(setting.pretty_write)
				{
					reserve += 1 + m_NodeData[i].m_Depth;
				}
			}
			break;
			case type_t::VALUE:
			{
				auto val = m_NodeData[i].as_value_content().value();
				reserve += (val.first) ? constants::LITERAL_OPEN.size() + constants::LITERAL_CLOSE.size() : 0;
			}
			break;
			case type_t::REFERENCE:
			{
				reserve += constants::REFERENCE.size();
			}
			break;
			case type_t::VALUE_RANGE:
			{
				auto val = m_NodeData[i].as_value_range_content().value();
				reserve += constants::RANGE_OPEN.size() + constants::RANGE_CLOSE.size();
				for(const auto& it : val)
				{
					reserve += (it.first) ? constants::LITERAL_OPEN.size() + constants::LITERAL_CLOSE.size() : 0;
					reserve += constants::RANGE_DIVIDER.size();
				}
			}
			break;
			case type_t::REFERENCE_RANGE:
			{
				reserve += constants::RANGE_OPEN.size() + constants::RANGE_CLOSE.size();
				auto val = m_NodeData[i].as_reference_range().value();
				reserve += val.size() * (constants::REFERENCE.size() + constants::RANGE_DIVIDER.size());
			}
			break;
			default:
			{
				throw std::runtime_error("malformed node");
			};
			}

			reserve += 1;
		}
		res.reserve(reserve);


		size_t checksum_index = 0;
		if(setting.write_header)
		{
			res += "VERSION " + std::to_string(m_Features.version.major) + "." +
				   std::to_string(m_Features.version.minor) + '\n';
			checksum_index = res.size();	// we need to insert into this location at the end
			res += "FEATURE_RECOVER_ON_ERROR " + std::to_string(m_Features.recover_from_error) + '\n';
		}


		size_t checksum_content_index = res.size();

		for(int i = 0; i < m_NodeData.size(); ++i)
		{
			m_NodeData[i].to_string(setting, res);
			res += ((setting.pretty_write) ? "\n" : "");
			if(m_NodeData[i].m_Type == type_t::COLLECTION)
			{
				i += m_NodeData[i].reinterpret_as_collection();
			}
		}
		res.erase(res.end() - 1);

		if(setting.write_header)
		{
			auto checksum =
			  utility::crc32(psl::string8::view {&res[checksum_content_index], res.size() - checksum_content_index});
			auto chksum_str = psl::string8_t("CHECKSUM " + std::to_string(checksum) + "\n");
			res.insert(res.begin() + checksum_index, chksum_str.begin(), chksum_str.end());
		}
		return res;
	}
}

bool container::contains(psl::string8::view name) const
{
	std::vector<psl::string8::view> partNames = utility::string::split(name, constants::NAMESPACE_DIVIDER);
	children_t depth						  = 0;
	for(int i = 0; i < m_NodeData.size(); ++i)
	{
		if(m_NodeData[i].m_Depth == depth && m_NodeData[i].name() == partNames[depth])
		{
			++depth;
			if(depth == partNames.size()) return true;
		}
		else if(m_NodeData[i].m_Type == type_t::COLLECTION)
		{
			i += m_NodeData[i].reinterpret_as_collection();
		}
	}
	return false;
}

psl::string8_t container::fullname(const data& node) const
{
	// todo possibly: cache all known collection indices, this way you can just jump to the next one instead of evalling
	// all nodes
	auto index = index_of(node);
	psl::string8_t name {node.name()};
	int depth = node.m_Depth;
	if(index > 0)
	{
		auto it = m_NodeData.begin() + index;
		do
		{
			--it;
			if(it->m_Type == type_t::COLLECTION)
			{
				if(it->m_Depth < depth)
				{
					--depth;
					name.insert(0, constants::NAMESPACE_DIVIDER);
					name.insert(0, it->name());
				}
			}

		} while(it != m_NodeData.begin() && depth != 0);
	}
	return name;
}

std::vector<data>::iterator
rfind_last_index_of(std::vector<type_t> types, std::vector<data>::iterator first, std::vector<data>::iterator last)
{
	for(auto it = last - 1; it != first; --it)
	{
		if(std::find(types.cbegin(), types.cend(), it->type()) != types.cend())
		{
			return it;
		}
	}
	return first;
}

std::vector<data>::iterator
find_first_index_of(std::vector<type_t> types, std::vector<data>::iterator first, std::vector<data>::iterator last)
{
	for(auto it = first; it != last; ++it)
	{
		if(std::find(types.cbegin(), types.cend(), it->type()) != types.cend())
		{
			return it;
		}
	}
	return last;
}

void container::move(data& node, nodes_t index)
{
	auto current_index = index_of(node);
	nodes_t move_count = 1;
	if(current_index < index)	 // forward move
	{
		switch(m_NodeData[current_index].m_Type)
		{
		case type_t::VALUE:
		{
			// auto [is_literal, content_info] = m_NodeData[current_index].reinterpret_as_value();
			// auto content_index = content_info.first;
			// auto content_size = content_info.second;
			// if (auto it = rfind_last_index_of({ type_t::VALUE, type_t::VALUE_RANGE }, m_NodeData.begin() +
			// current_index, m_NodeData.begin() + index); it != m_NodeData.begin() + current_index)
			//{
			//	//auto last_value_node = it - m_NodeData.begin();
			//	if (it->m_Type == type_t::VALUE)
			//	{
			//	}
			//	else
			//	{

			//	}
			//}
		}
		break;
		case type_t::VALUE_RANGE:
		{
		}
		break;
		case type_t::COLLECTION:
		{
			move_count += m_NodeData[current_index].reinterpret_as_collection();
		}
		break;
		default:
		{
		};
		}
		std::rotate(m_NodeData.begin() + current_index,
					m_NodeData.begin() + current_index + move_count,
					m_NodeData.begin() + index);
	}
	else if(current_index > index)	  // back move
	{
		std::rotate(m_NodeData.rbegin() + current_index,
					m_NodeData.rbegin() + current_index + move_count,
					m_NodeData.rbegin() + index);
	}
}

void container::validate()
{
	nodes_t expected_index		= 0;
	size_t last_name_index_n	= 0u;
	size_t last_content_index_n = 0u;
	for(const auto& it : m_NodeData)
	{
		bool bShouldThrow = it.m_Handle->m_Index != expected_index;
		bShouldThrow |= it.m_Name.first != last_name_index_n;

		switch(it.m_Type)
		{
		case type_t::VALUE:
		{
			const auto& val = it.reinterpret_as_value();
			bShouldThrow |= val.second.first != last_content_index_n;
			last_content_index_n += val.second.second;
		}
		break;
		case type_t::VALUE_RANGE:
		{
			const auto& val = it.reinterpret_as_value_range();
			for(const auto& valIt : val)
			{
				bShouldThrow |= valIt.second.first != last_content_index_n;
				last_content_index_n += valIt.second.second;
			}
		}
		break;
		default:
		{
		};
		}

		if(bShouldThrow) throw new std::exception();
		++expected_index;
		last_name_index_n += it.m_Name.second;
	}
}

handle& container::find(const data& parent, psl::string8::view name)
{
	auto index = index_of(parent, name);
	if(std::numeric_limits<nodes_t>::max() == index) return m_TerminalHandle;
	return *m_NodeData[index].m_Handle;
}

handle& container::find(psl::string8::view name)
{
	auto index = index_of(name);
	if(std::numeric_limits<nodes_t>::max() == index) return m_TerminalHandle;
	return *m_NodeData[index].m_Handle;
}

container::container(container&& other) :
	m_NodeData(std::move(other.m_NodeData)), m_Content(std::move(other.m_Content)),
	m_InternalData(std::move(other.m_InternalData)), m_Features(other.m_Features)
{
	other.m_NodeData.clear();
};

container& container::operator=(container&& other)
{
	if(this != &other)
	{
		m_NodeData	   = std::move(other.m_NodeData);
		m_Content	   = std::move(other.m_Content);
		m_InternalData = std::move(other.m_InternalData);
		m_Features	   = other.m_Features;
		other.m_NodeData.clear();
	}
	return *this;
};

data& handle::get() const { return m_Container->m_NodeData[m_Index]; };

data* handle::operator->() const { return &m_Container->m_NodeData[m_Index]; };

char const* node_not_found::what() const noexcept
{
	psl::string8_t message {
	  "An unknown error occured, please create a repro case and submit as a bug ticket, thank you!"};
	if(auto index = std::get_if<nodes_t>(&m_Data))
	{
		if((size_t)*index >= m_Container->size())
		{
			message =
			  "The index " + utility::to_string(*index) +
			  " is larger than the last element, which is at index: " + utility::to_string(m_Container->size() - 1);
		}
	}
	else if(auto name = std::get_if<psl::string8_t>(&m_Data))
	{
		message = "The node named '" + *name + "' could not be found in the container.";
	}
	m_Message = std::move(message);
	return m_Message.data();
}
