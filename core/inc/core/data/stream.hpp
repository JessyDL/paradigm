#pragma once
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/math/matrix.hpp"
#include "psl/math/vec.hpp"
#include "psl/serialization/serializer.hpp"

namespace core {
namespace details {
	enum class memory_stream_type_e { single = 0, vec2 = 1, vec3 = 2, vec4 = 3, mat2 = 4, mat3x3 = 5, mat4x4 = 6 };

	template <typename T>
	struct memory_stream_underlying_from_type {
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, psl::vec2> || std::is_same_v<T, psl::vec3> ||
					  std::is_same_v<T, psl::vec4> || std::is_same_v<T, psl::mat2x2> ||
					  std::is_same_v<T, psl::mat3x3> || std::is_same_v<T, psl::mat4x4>);
		using type									= T;
		static constexpr memory_stream_type_e value = std::is_same_v<T, float>		   ? memory_stream_type_e::single
													  : std::is_same_v<T, psl::vec2>   ? memory_stream_type_e::vec2
													  : std::is_same_v<T, psl::vec3>   ? memory_stream_type_e::vec3
													  : std::is_same_v<T, psl::vec4>   ? memory_stream_type_e::vec4
													  : std::is_same_v<T, psl::mat2x2> ? memory_stream_type_e::mat2
													  : std::is_same_v<T, psl::mat3x3> ? memory_stream_type_e::mat3x3
																					   : memory_stream_type_e::mat4x4;
	};

	template <typename T>
	static constexpr auto memory_stream_underlying_from_type_v = memory_stream_underlying_from_type<T>::value;

	template <memory_stream_type_e Type>
	struct memory_stream_underlying_from_enum {
	  public:
		static_assert(Type == memory_stream_type_e::single || Type == memory_stream_type_e::vec2 ||
						Type == memory_stream_type_e::vec3 || Type == memory_stream_type_e::vec4 ||
						Type == memory_stream_type_e::mat2 || Type == memory_stream_type_e::mat3x3 ||
						Type == memory_stream_type_e::mat4x4,
					  "unhandled type");
		using type = std::conditional_t<
		  Type == memory_stream_type_e::single,
		  float,
		  std::conditional_t<
			Type == memory_stream_type_e::vec2,
			psl::vec2,
			std::conditional_t<
			  Type == memory_stream_type_e::vec3,
			  psl::vec3,
			  std::conditional_t<
				Type == memory_stream_type_e::vec4,
				psl::vec4,
				std::conditional_t<
				  Type == memory_stream_type_e::mat2,
				  psl::mat2x2,
				  std::conditional_t<Type == memory_stream_type_e::mat3x3,
									 psl::mat3x3,
									 std::conditional_t<Type == memory_stream_type_e::mat4x4, psl::mat4x4, void>>>>>>>;
		static constexpr auto value = Type;
	};

	template <memory_stream_type_e Type>
	using memory_stream_underlying_from_enum_t = typename memory_stream_underlying_from_enum<Type>::type;

	class memory_stream_base_t {
	  public:
		virtual void* data() noexcept			  = 0;
		virtual const void* data() const noexcept = 0;
		virtual size_t size() const noexcept	  = 0;
	};

	template <memory_stream_type_e Type>
	class memory_stream_impl_t : memory_stream_base_t {
	  public:
		using unit_t							 = memory_stream_underlying_from_enum_t<Type>;
		static constexpr auto memory_stream_type = Type;

		template <typename... Ts>
		memory_stream_impl_t(Ts&&... data) : m_Data(std::forward<Ts>(data)...) {}

		void* data() noexcept override { return (void*)m_Data.data(); }
		const void* data() const noexcept override { return (void*)m_Data.data(); }
		size_t size() const noexcept override { return m_Data.size(); }
		size_t element_size() const noexcept { return sizeof(unit_t); }

		auto cbegin() const noexcept { return m_Data.cbegin(); }
		auto begin() const noexcept { return m_Data.begin(); }
		auto begin() noexcept { return m_Data.begin(); }
		auto cend() const noexcept { return m_Data.cend(); }
		auto end() const noexcept { return m_Data.end(); }
		auto end() noexcept { return m_Data.end(); }

		auto& value() noexcept { return m_Data; }
		const auto& value() const noexcept { return m_Data; }

		size_t components() const noexcept { return sizeof(unit_t) / sizeof(float); }

	  private:
		psl::array<unit_t> m_Data;
	};
}	 // namespace details

/// \brief vertex_stream_t contains a type erased "stream" of memory with basic facilities to protect it from incorrect
/// casts.
///
/// stream should be used in scenarios where something could contain various, but functionaly similar arrays of
/// float. see core::data::geometry_t for an example of this. \see core::data::geometry_t
class vertex_stream_t {
	friend class psl::serialization::accessor;

  public:
	/// \brief the supported internal types of the stream.
	using type = details::memory_stream_type_e;

	vertex_stream_t(type type = type::single) { init(type); }

	template <typename T>
	vertex_stream_t(psl::array<T>&& data) {
		m_Stream = details::memory_stream_impl_t<details::memory_stream_underlying_from_type_v<T>>(
		  std::forward<psl::array<T>>(data));
	}

	/// \brief returns the type safe variant of the stream as a reference.
	///
	/// if the stream is of the correct type, it will return a valid vector, otherwise an assert is raised
	template <type Type>
	auto& get() noexcept {
		psl_assert(std::holds_alternative<details::memory_stream_impl_t<Type>>(m_Stream),
				   "Incorrect type requested compared to the underlying type in the stream");
		return std::get<details::memory_stream_impl_t<Type>>(m_Stream).value();
	}

	/// \brief returns the type safe variant of the stream as a reference.
	///
	/// if the stream is of the correct type, it will return a valid vector, otherwise an assert is raised
	template <type Type>
	const auto& get() const noexcept {
		psl_assert(std::holds_alternative<details::memory_stream_impl_t<Type>>(m_Stream),
				   "Incorrect type requested compared to the underlying type in the stream");
		return std::get<details::memory_stream_impl_t<Type>>(m_Stream).value();
	}

	template <typename F>
	bool transform(F&& transformation) {
		return std::visit(
		  [&transformation]<typename T>(T& stream) {
			  if constexpr(std::is_invocable_v<F, typename T::unit_t&>) {
				  std::for_each(std::begin(stream.value()), std::end(stream.value()), transformation);
				  return true;
			  }
			  return false;
		  },
		  m_Stream);
	}

	template <typename T>
	bool is() const noexcept {
		return std::visit([]<typename Y>(const Y& value) { return std::is_same_v<T, typename Y::unit_t>; }, m_Stream);
	}

	/// \brief returns the size of the stream (in unique element count).
	///
	/// depending on the contained type, one "element" is bigger/smaller. This method returns the size of one
	/// element. for example if the stream contains vec2 data, one element is equal to 2 floats, and so the size()
	/// would be 2 if there were 4 floats present. similarly if the stream contained vec3 data, one element would be
	/// equivalent to 3 floats, and so a size() of 4 would be equal to 12 floats. \returns the element count.
	size_t size() const noexcept {
		return std::visit([](const auto& val) { return val.size(); }, m_Stream);
	}

	/// \brief returns the total size of the memory stream in bytes.
	/// \returns the total size of the memory stream in bytes.
	size_t bytesize() const noexcept {
		return std::visit([](const auto& val) { return val.size() * val.element_size(); }, m_Stream);
	}

	size_t elements() const noexcept {
		return std::visit([](const auto& value) { return value.components(); }, m_Stream);
	}

	/// \brief returns the pointer to the head of the memory stream.
	/// \returns the pointer to the head of the memory stream.
	void* data() noexcept {
		return std::visit([](auto& val) { return val.data(); }, m_Stream);
	}

	/// \brief returns the constant pointer to the head of the memory stream.
	/// \returns the constant pointer to the head of the memory stream.
	const void* data() const noexcept {
		return std::visit([](const auto& val) { return val.data(); }, m_Stream);
	}

	void resize_bytecount(size_t bytes) {
		std::visit([&bytes](auto& val) { return val.value().resize(bytes / val.element_size()); }, m_Stream);
	};
	void resize_elementcount(size_t elements) {
		std::visit([&elements](auto& val) { return val.value().resize(elements); }, m_Stream);
	};

  private:
	inline void init(type t) {
		switch(t) {
		case type::single:
			m_Stream = details::memory_stream_impl_t<type::single> {};
			break;
		case type::vec2:
			m_Stream = details::memory_stream_impl_t<type::vec2> {};
			break;
		case type::vec3:
			m_Stream = details::memory_stream_impl_t<type::vec3> {};
			break;
		case type::vec4:
			m_Stream = details::memory_stream_impl_t<type::vec4> {};
			break;
		case type::mat2:
			m_Stream = details::memory_stream_impl_t<type::mat2> {};
			break;
		case type::mat3x3:
			m_Stream = details::memory_stream_impl_t<type::mat3x3> {};
			break;
		case type::mat4x4:
			m_Stream = details::memory_stream_impl_t<type::mat4x4> {};
			break;
		default:
			throw std::runtime_error("unhandled stream type");
		}
	}
	template <typename S>
	void serialize(S& serializer) {
		// loading and storing are differing codepaths due to performance, and safety reasons.
		// as the underlying type is a variant, we need to figure out the type beforehand so we can prime the
		// correct variant to be loaded. For performance reasons we prefer as little copying as needed, so for
		// storing we transform it into an array_view, and loading we have the destination as a reference to.
		if constexpr(psl::serialization::details::IsDecoder<S>)	   // load from disk
		{
			psl::serialization::property<"TYPE", type> type;
			serializer << type;
			init(type);
			std::visit(
			  [&serializer](auto& value) {
				  psl::serialization::property<"DATA", psl::array<typename decltype(value)::unit_t>&> data;
				  serializer << data;
			  },
			  m_Stream);
		} else	  // storing to disk
		{
			std::visit(
			  [&serializer]<typename T>(T& value) {
				  psl::serialization::property<"TYPE", type> typeProp {T::memory_stream_type};
				  serializer << typeProp;
				  serializer.template parse<"DATA">(value.value());
			  },
			  m_Stream);
		}
	};

	static constexpr psl::string8::view serialization_name {"CORE_STREAM"};

	std::variant<details::memory_stream_impl_t<type::single>,
				 details::memory_stream_impl_t<type::vec2>,
				 details::memory_stream_impl_t<type::vec3>,
				 details::memory_stream_impl_t<type::vec4>,
				 details::memory_stream_impl_t<type::mat2>,
				 details::memory_stream_impl_t<type::mat3x3>,
				 details::memory_stream_impl_t<type::mat4x4>>
	  m_Stream;
};
}	 // namespace core
