#pragma once
#include "range.hpp"
#include "segment.hpp"
#include <cmath>
#include <list>
#include <optional>
#include <stack>
#include <vector>

namespace memory {
class region;
class range_t;
/// \brief base class that defines the interface for an allocator.
class allocator_base {
	friend class region;

  public:
	allocator_base(bool physically_backed = true) : m_IsPhysicallyBacked(physically_backed) {};
	virtual ~allocator_base() = default;

	allocator_base(const allocator_base&)			 = delete;
	allocator_base& operator=(const allocator_base&) = delete;

	[[nodiscard]] std::optional<segment> allocate(std::size_t bytes) {
		if(bytes == 0)
			return std::nullopt;
		return do_allocate(m_Region, bytes);
	};

	bool deallocate(segment& segment);

	std::vector<range_t> committed();
	std::vector<range_t> available();
	bool is_physically_backed() const noexcept { return m_IsPhysicallyBacked; };

	size_t alignment() const noexcept;

	bool owns(const memory::segment& segment) const noexcept;

	void compact();

  protected:
	bool commit(const range_t& range);
	memory::range_t get_range() const;

  private:
	region* m_Region {nullptr};
	virtual std::optional<segment> do_allocate(region* region, std::size_t bytes) = 0;
	virtual bool do_deallocate(segment& segment)								  = 0;
	virtual void initialize([[maybe_unused]] region* region) {};
	virtual std::vector<range_t> get_committed() const = 0;
	virtual std::vector<range_t> get_available() const = 0;
	virtual void do_compact([[maybe_unused]] region* region) {};
	virtual bool get_owns(const memory::segment& segment) const noexcept = 0;
	const bool m_IsPhysicallyBacked {true};
};

/// \brief default allocator that internally works using lists
class default_allocator : public allocator_base {
  public:
	default_allocator(bool physically_backed = true) : allocator_base(physically_backed) {};
	virtual ~default_allocator() = default;

  private:
	std::optional<segment> do_allocate(region* region, std::size_t bytes) override;
	bool do_deallocate(segment& segment) override;
	void initialize(region* region) override;
	std::vector<range_t> get_committed() const override;
	std::vector<range_t> get_available() const override;
	void do_compact(region* region) override;
	bool get_owns(const memory::segment& segment) const noexcept override;
	std::list<range_t> m_Committed;
	std::list<range_t> m_Free;
};

/// \brief predifined block size allocator, much faster than most allocators, but can only allocate one sized
/// blocks.
class block_allocator : public allocator_base {
  public:
	block_allocator(size_t block_size, bool physically_backed = true)
		: allocator_base(physically_backed), m_BlockSize(block_size) {};
	virtual ~block_allocator() = default;

  private:
	std::optional<segment> do_allocate(region* region, std::size_t bytes) override;
	bool do_deallocate(segment& segment) override;
	void initialize(region* region) override;

	std::vector<range_t> get_committed() const override;
	std::vector<range_t> get_available() const override;

	bool get_owns(const memory::segment& segment) const noexcept override;

	std::vector<memory::range_t> m_Ranges;
	std::stack<size_t> m_Free;
	const size_t m_BlockSize;
};
}	 // namespace memory
