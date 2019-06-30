#pragma once
#include "array.h"
#include "bytell_map.h"
#include "ustring.h"
#include "timer.h"

#if defined(_MSC_VER)
	#define FUNCTION_SIGNATURE_INFO __FUNCSIG__
#else
	#define FUNCTION_SIGNATURE_INFO __PRETTY_FUNCTION__
#endif

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#ifdef PE_PROFILER
#define PROFILE_SCOPE(profiler) volatile auto TOKENPASTE2(prof_, __LINE__) { std::move(profiler.scope(psl::string(FUNCTION_SIGNATURE_INFO), (void*)this)) };
#define PROFILE_SCOPE_STATIC(profiler) volatile auto TOKENPASTE2(prof_, __LINE__) { std::move(profiler.scope(psl::string(FUNCTION_SIGNATURE_INFO))) };
#define PROFILE_SCOPE_BEGIN(profiler) profiler.scope_begin(psl::string(FUNCTION_SIGNATURE_INFO), (void*)this);
#define PROFILE_SCOPE_END(profiler) profiler.scope_end((void*)this);
#else
#define PROFILE_SCOPE(profiler)
#define PROFILE_SCOPE_STATIC(profiler)
#define PROFILE_SCOPE_BEGIN(profiler)
#define PROFILE_SCOPE_END(profiler)
#endif
namespace psl::profiling
{
	class profiler
	{
	struct scoped_block
	{
		scoped_block(profiler& profiler) noexcept;
		~scoped_block() noexcept;
		scoped_block(const scoped_block&) = delete;
		scoped_block(scoped_block&& other);
		scoped_block(volatile scoped_block&& other);
		scoped_block& operator=(const scoped_block&) = delete;
		scoped_block& operator=(scoped_block&&) = delete;
	private:
		profiler* prf;
	};

	struct scope_info
	{
		scope_info(uint64_t name, size_t depth, bool mangled_name = false) : name(name), duration(0), depth(depth), mangled_name(mangled_name){};
		uint64_t name;
		bool mangled_name;
		std::chrono::microseconds duration;
		size_t depth;
	};

	struct frame_info
	{
		void push(const psl::string& name) noexcept;
		void push(uint64_t name) noexcept;
		void pop() noexcept;
		void clear();
		void end();
		psl::vector<scope_info> m_Scopes{};
		psl::timer m_Timer{};
		std::chrono::microseconds duration;
		size_t m_Stack{0};
		psl::bytell_map<psl::string, uint64_t> m_NameMap;
		psl::bytell_map<uint64_t, psl::string> m_IDMap;
		uint64_t IDCounter{0};
	};

	public:
		profiler(size_t buffer_size = 5);
		void next_frame();
		volatile scoped_block scope(const psl::string& name) noexcept;
		volatile scoped_block scope() noexcept;
		volatile scoped_block scope(void* target) noexcept;
		volatile scoped_block scope(const psl::string& name, void* target) noexcept;
		void scope_begin(const psl::string& name);
		void scope_begin(const psl::string& name, void* target);
		void scope_end();
		void scope_end(void* target);

		psl::string to_string() const;
	private:
		psl::vector<frame_info> m_Frames;
		size_t m_FrameIndex;
	};
}