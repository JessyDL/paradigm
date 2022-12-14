#include "psl/formatted_string_buffer.hpp"
#include "psl/logging.hpp"
#include <numeric>

formatted_string_buffer::formatted_string_buffer() {}

formatted_string_buffer::formatted_string_buffer(const psl::string8_t& dividerOpen, const psl::string8_t& dividerClose)
	: hasDivider(true), dividerOpen(dividerOpen), dividerClose(dividerClose) {}


formatted_string_buffer::~formatted_string_buffer() {}

void formatted_string_buffer::emplace_back(const psl::string8_t& text) {
	m_Buffer.emplace_back(psl::string8_t(m_IndentLevel, '\t') + text);
}

void formatted_string_buffer::IncreaseIndent() {
	if(m_IndentLevel != std::numeric_limits<unsigned int>::max()) {
		if(hasDivider)
			emplace_back(dividerOpen);
		m_IndentLevel += 1;
	} else
		LOG_WARNING("Tried to increase indent beyond the numeric limit");
}

void formatted_string_buffer::DecreaseIndent() {
	if(m_IndentLevel != 0) {
		m_IndentLevel -= 1;
		if(hasDivider)
			emplace_back(dividerClose);
	} else
		LOG_WARNING("Tried to decrease indent beyond the numeric limit");
}

psl::string8_t formatted_string_buffer::ToString() const {
	psl::string8_t result;
	auto append_string = [](psl::string8_t accum, const psl::string8_t& entry) { return accum + entry + '\n'; };
	return std::accumulate(std::begin(m_Buffer), std::end(m_Buffer), psl::string8_t {}, append_string);
}
