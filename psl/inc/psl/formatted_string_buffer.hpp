#pragma once
#include "psl/ustring.hpp"
#include <vector>

class formatted_string_buffer
{
  public:
	formatted_string_buffer();
	formatted_string_buffer(const psl::string8_t& dividerOpen, const psl::string8_t& dividerClose);
	~formatted_string_buffer();

	void emplace_back(const psl::string8_t& text);
	void IncreaseIndent();
	void DecreaseIndent();
	psl::string8_t ToString() const;


  private:
	std::vector<psl::string8_t> m_Buffer {};
	unsigned int m_IndentLevel {0};
	bool hasDivider {false};
	psl::string8_t dividerOpen;
	psl::string8_t dividerClose;
};
