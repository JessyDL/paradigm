#pragma once
#include <cstdint>
#include <stddef.h>

namespace memory {
class raw_region {
  public:
	raw_region(size_t size);
	~raw_region();
	raw_region(const raw_region& other);
	raw_region(raw_region&& other);
	raw_region& operator=(const raw_region& other);
	raw_region& operator=(raw_region&& other);

	void* data() const noexcept { return m_Base; }
	size_t size() const noexcept { return m_Size; }
	size_t pageSize() const noexcept { return m_PageSize; }

  private:
	void release() noexcept;
	void* m_Base {nullptr};
	size_t m_Size {0u};
	size_t m_PageSize {0u};
};
}	 // namespace memory
