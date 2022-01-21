#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"
#include "psl/memory/region.hpp"
#include "psl/memory/segment.hpp"
#include "psl/serialization/serializer.hpp"
#include <optional>
#include <vector>

/// \brief contains all data types that can be serialized to/from disk.
namespace core::data
{
	/// \brief container class for GPU data.
	///
	/// core::data::buffer_t is a data container for anything that will be uploaded to the GPU. This means that this can
	/// not contain any complex types (such as indirections). core::data::buffer_t can be incorrectly set up when giving
	/// incompatible memory::region bundled with core::gfx::memory_usage. \note the memory::region you pass to this
	/// object will also dictate the **size** and **alignment** requirements of this specific resource on the GPU. \todo
	/// figure out a way around incompatible core::data::buffer_t setups, perhaps by using structs to construct the class.
	/// \author Jessy De Lannoit
	class buffer_t
	{
		friend class psl::serialization::accessor;

	  public:
		/// will do the minimal setup needed, no allocations happen at this point yet.
		/// \param[in] uid the psl::UID that is assigned to this object
		/// \param[in] cache which cache this object has been allocated in
		/// \param[in] usage the usage flags that signify how the resource should be used by the GPU
		/// \param[in] memoryPropertyFlags what are the properties of the memory (i.e. where does it live)
		/// \param[in] memory_region what is the owning region of this memory. Note that this parameter also will
		/// dictate the size and alignment of the resource.
		buffer_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile,
			   core::gfx::memory_usage usage,
			   core::gfx::memory_property memoryPropertyFlags,
			   memory::region&& memory_region) noexcept;

		~buffer_t();

		buffer_t(const buffer_t&) = delete;
		/*buffer(buffer&& other)
			: m_Region(std::move(other.m_Region)), m_Segments(std::move(other.m_Segments)), m_Usage(other.m_Usage),
			  m_MemoryPropertyFlags(other.m_MemoryPropertyFlags){

			  };*/
		buffer_t& operator=(const buffer_t&) = delete;
		buffer_t& operator=(buffer_t&&) = delete;

		/// \returns the total size in the memory::region that this buffer occupies.
		size_t size() const;

		/// \returns the core::gfx::memory_usage of this instance, showing the type of resource this is for the GPU
		core::gfx::memory_usage usage() const;

		/// \returns the core::gfx::memory_property, so you know where the data lives.
		core::gfx::memory_property memoryPropertyFlags() const;

		/// \returns the associated memory::region, where the data lives.
		/// \note the data can exist "virtually" as well, for example in the case this is a pure GPU resource.
		const memory::region& region() const;

		/// \returns all committed memory::segments in the memory::region. Using this you can calculate fragmentation,
		/// and allocated size.
		const std::vector<memory::segment>& segments() const;

		/// Will try to allocate a new memory::segment of _atleast_ the given size.
		/// This method will adhere to the alignment requirements of the underlaying memory::region.
		///
		/// \returns optionally, a new allocated memory::segment of _atleast_ the given size.
		/// \param[in] size the minimum expected size, this will auto-align/grow to the alignment requirements.
		std::optional<memory::segment> allocate(size_t size);

		/// \returns true in case the deallocation was successful.
		/// \param[in] segment the target segment to deallocate.
		/// \note \a segment gets invalidated on success.
		bool deallocate(memory::segment& segment);

		bool transient() const noexcept;
		void transient(bool value) noexcept;
		core::gfx::memory_write_frequency write_frequency() const noexcept;
		void write_frequency(core::gfx::memory_write_frequency value) noexcept;

	  private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Usage << m_MemoryPropertyFlags << m_Transient << m_WriteFrequency;

			if constexpr(psl::serialization::details::IsEncoder<S>)
			{
				psl::serialization::property<"SIZE", size_t> size {m_Region.size()};
				psl::serialization::property<"ALIGNMENT", size_t> alignment {m_Region.alignment()};
				psl::serialization::property<"DATA", std::vector<psl::string8_t>> data {};

				for(auto it : m_Segments)
				{
					data.value.emplace_back((char*)it.range().begin, it.range().size());
				}

				serializer << size << alignment << data;
			}
			else
			{
				psl::serialization::property<"SIZE", size_t> size {0u};
				psl::serialization::property<"ALIGNMENT", size_t> alignment {4u};
				psl::serialization::property<"DATA", std::vector<psl::string8_t>> data {};

				serializer << size << alignment << data;
				for(auto it : data.value)
				{
					if(auto segm = m_Region.allocate(it.size()); segm)
					{
						segm.value().unsafe_set(it.data(), it.size());
						m_Segments.push_back(segm.value());
					}
					else
						LOG_ERROR("Could not allocate a segment on the region of the size specified.");
				}
			}
		}

		static constexpr const char serialization_name[7] {"BUFFER"};

		memory::region m_Region;
		std::vector<memory::segment> m_Segments;
		psl::serialization::property<"TRANSIENT", bool> m_Transient {
		  false};	 // is the buffer short lived (true) or not (false)
		psl::serialization::property<"WRITE FREQUENCY", core::gfx::memory_write_frequency> m_WriteFrequency {
		  core::gfx::memory_write_frequency::per_frame};	// is the buffer's data changing
															// frequently (false) or not (true)
		psl::serialization::property<"USAGE", core::gfx::memory_usage> m_Usage;
		psl::serialization::property<"PROPERTIES", core::gfx::memory_property> m_MemoryPropertyFlags;
	};
}	 // namespace core::data
