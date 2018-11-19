#pragma once
#include "IDGenerator.h"

namespace core::data
{
	class material;
}
namespace core::gfx
{
	class pipeline;
	class pipeline_cache;
	class context;
	class shader;
	class texture;
	class sampler;
	class buffer;
	class framebuffer;
	class swapchain;
	class geometry;

	/// \brief class that creates a bindable collection of resources that can be used in conjuction with a surface to
	/// render.
	///
	/// The material class is a container of various resources that can, together, describe what should
	/// happen to a surface in the render pipeline on the GPU, and what is all needed.
	/// The material class also can contain instance data and will manage this for you.
	/// Together with a core::gfx::geometry, this describes all the resources you need to render something on screen.
	class material final
	{
		template <typename T>
		using optional_ref = std::optional<std::reference_wrapper<T>>;

		template <typename T, bool use_custom_uid = false>
		using dependency = core::resource::dependency<T, use_custom_uid>;

		template <typename... Ts>
		using packet = core::resource::packet<Ts...>;

		struct instance_element
		{
			uint32_t slot;
			uint32_t size_of_element;
			psl::string name;
		};

		struct instance_object_data
		{
			uint32_t slot;
			uint32_t size_of_element;
			memory::segment segment;
		};

		struct instance_object
		{
			instance_object(uint32_t capacity = 0u, size_t elements = 0) : segments({}), id_generator(capacity), size(0u) { segments.reserve(elements);};
			~instance_object() = default;
			instance_object(const instance_object& other) : segments(other.segments), id_generator(other.id_generator), size(other.size) {};
			instance_object(instance_object&&) = default;
			instance_object& operator=(const instance_object&) = default;
			instance_object& operator=(instance_object&&) = default;

			std::vector<instance_object_data> segments;
			psl::IDGenerator<uint32_t> id_generator;
			uint32_t size;
		};

		class instance_data
		{
		  public:
			instance_data(std::vector<instance_element>&& elements, uint32_t size)
				: elements(std::move(elements)), m_Capacity(size), m_Instance({}){};
			instance_data()						= default;
			~instance_data()					= default;
			instance_data(const instance_data&) = default;
			instance_data(instance_data&&)		= default;
			instance_data& operator=(const instance_data&) = default;
			instance_data& operator=(instance_data&&) = default;

			std::optional<uint32_t> add(core::resource::handle<core::gfx::buffer> buffer, const UID& uid);

			bool remove(core::resource::handle<core::gfx::buffer> buffer, const UID& uid, uint32_t id);

			bool has_data() const noexcept { return elements.size() > 0; }

			optional_ref<const instance_element> has_element(psl::string_view name) const noexcept;

			optional_ref<const instance_element> has_element(uint32_t slot) const noexcept;
			optional_ref<instance_object> instance(const UID& uid) noexcept;
			uint32_t size(const UID& uid) const noexcept;

			const auto& begin() const { return std::begin(elements); }

			const auto& end() const { return std::end(elements); }

			bool remove_all(core::resource::handle<core::gfx::buffer> buffer);
			bool remove_all(core::resource::handle<core::gfx::buffer> buffer, const UID& uid);
		  private:
			std::vector<instance_element> elements;
			std::unordered_map<UID, instance_object> m_Instance;
			uint32_t m_Capacity;
		};

	  public:
		using resource_dependency = packet<dependency<core::data::material, true>, UID, core::resource::cache>;

		/// \brief the constructor that will create and bind the necesary resources to create a valid pipeline.
		/// \param[in] packet resource packet containing the data that is needed from the resource system.
		/// \param[in] context a handle to a graphics context (needs to be valid and loaded) which will own the
		/// material.
		/// \param[in] data the material data this instance will be based on.
		/// \param[in] pipeline_cache the pipeline_cache this instance can request a pipeline from.
		/// \param[in] materialBuffer a GPU buffer that can be used by this instance to upload data to (if needed).
		/// \param[in] instanceBuffer a GPU buffer that can be used to upload instance data to, if there is any.
		material(resource_dependency packet, core::resource::handle<core::gfx::context> context,
				 core::resource::handle<core::data::material> data,
				 core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
				 core::resource::handle<core::gfx::buffer> materialBuffer,
				 core::resource::handle<core::gfx::buffer> instanceBuffer);
		material()				  = delete;
		~material();
		material(const material&) = delete;
		material(material&&)	  = delete;
		material& operator=(const material&) = delete;
		material& operator=(material&&) = delete;

		/// \brief returns a handle to the material data used to construct this object.
		/// \note when editing the material or the data after construction, this value will be out of sync with the
		/// runtime gfx::material.
		core::resource::handle<core::data::material> data() const;
		/// \brief returns all the shaders that are being used right now by this material.
		const std::vector<core::resource::handle<core::gfx::shader>>& shaders() const;
		/// \brief returns all currently used textures and their binding slots.
		const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::texture>>>& textures() const;
		/// \brief returns all currently used samplers and their binding slots.
		const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::sampler>>>& samplers() const;
		/// \brief returns all currently used buffers and their binding slots.
		/// \note the buffers could be anything, they could be uniform buffer objects, or maybe shader storage buffer
		/// objects.
		const std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::buffer>>>& buffers() const;

		/// \brief prepares the material for rendering by binding the pipeline.
		/// \warning only call this in the context of recording the draw call.
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] framebuffer the framebuffer the pipeline will be bound to.
		/// \param[in] drawIndex the index to be set in the push constant.
		/// \todo drawindex is a temporary hack to support instancing. a generic solution should be sought after.
		bool bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<framebuffer> framebuffer,
						   uint32_t drawIndex);

		/// \brief prepares the material for rendering by binding the pipeline.
		/// \warning only call this in the context of recording the draw call.
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] swapchain the swapchain the pipeline will be bound to.
		/// \param[in] drawIndex the index to be set in the push constant.
		bool bind_pipeline(vk::CommandBuffer cmdBuffer, core::resource::handle<swapchain> swapchain,
						   uint32_t drawIndex);

		/// \brief prepares the material for rendering by binding the geometry's instance data.
		/// \warning only call this in the context of recording the draw call, *after* you called bind_pipeline().
		/// \param[in] cmdBuffer the command buffer you'll be recording to
		/// \param[in] geometry the geometry that will be bound.
		/// \todo write instance buffer support.
		bool bind_geometry(vk::CommandBuffer cmdBuffer, const core::resource::handle<core::gfx::geometry> geometry);

		/// \brief returns the instance count currently used for the given piece of geometry.
		/// \param[in] geometry the geometry to check.
		uint32_t instances(const core::resource::handle<core::gfx::geometry> geometry) const;


		std::optional<uint32_t> instantiate(const core::resource::tag<core::gfx::geometry>& geometry);
		bool release(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id);
		bool release_all();
		template <typename T>
		bool set(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id, psl::string_view name,
				 const T& value)
		{
			static_assert(std::is_trivially_copyable<T>::value, "the type has to be trivially copyable");
			static_assert(std::is_standard_layout<T>::value, "the type has to be is_standard_layout");
			if(auto res = m_InstanceData.has_element(name); res)
			{
				return set(geometry, id, res.value().get().slot, &value, sizeof(T));
			}
			return false;
		};

		template <typename T>
		bool set(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id_first, psl::string_view name,
				 const std::vector<T>& values)
		{
			static_assert(std::is_trivially_copyable<T>::value, "the type has to be trivially copyable");
			static_assert(std::is_standard_layout<T>::value, "the type has to be is_standard_layout");
			if(auto res = m_InstanceData.has_element(name); res)
			{
				return set(geometry, id_first, res.value().get().slot, values.data(), sizeof(T), values.size());
			}
			return false;
		}

		template <typename T>
		bool set(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id, uint32_t binding,
				 const T& value)
		{
			static_assert(std::is_trivially_copyable<T>::value, "the type has to be trivially copyable");
			static_assert(std::is_standard_layout<T>::value, "the type has to be is_standard_layout");
			if(auto res = m_InstanceData.has_element(binding); res)
			{
				return set(geometry, id, res.value().get().slot, &value, sizeof(T));
			}
			return false;
		};

	  private:
		bool set(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id, uint32_t binding, const void* data,
				 size_t size);

		bool set(const core::resource::tag<core::gfx::geometry>& geometry, uint32_t id_first, uint32_t binding, const void* data,
				 size_t size, size_t count);
		/// \returns the pipeline this material instance uses for the given framebuffer.
		/// \details tries to find, and return a core::gfx::pipeline that can satisfy the
		/// requirements of this material. In case none is present, then one will be created instead.
		/// \param[in] framebuffer the framebuffer to bind to.
		core::resource::handle<pipeline> get(core::resource::handle<framebuffer> framebuffer);

		/// \returns the pipeline this material instance uses for the given framebuffer.
		/// \details tries to find, and return a core::gfx::pipeline that can satisfy the
		/// requirements of this material. In case none is present, then one will be created instead.
		/// \param[in] swapchain the swapchain to bind to.
		core::resource::handle<pipeline> get(core::resource::handle<swapchain> swapchain);

		core::resource::handle<core::gfx::context> m_Context;
		core::resource::handle<core::gfx::pipeline_cache> m_PipelineCache;
		core::resource::handle<core::data::material> m_Data;

		std::vector<core::resource::handle<core::gfx::shader>> m_Shaders;

		// a combination of binding slot + resource
		std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::texture>>> m_Textures;
		std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::sampler>>> m_Samplers;
		std::vector<std::pair<uint32_t, core::resource::handle<core::gfx::buffer>>> m_Buffers;

		core::resource::handle<core::gfx::buffer> m_MaterialBuffer;
		core::resource::handle<core::gfx::buffer> m_InstanceBuffer;

		// UID maps to the UID of a framebuffer or a swapchain
		std::unordered_map<UID, core::resource::handle<core::gfx::pipeline>> m_Pipeline;
		core::resource::handle<core::gfx::pipeline> m_Bound;

		instance_data m_InstanceData;
		/* m_MaterialData*/

		// value to indicate if this material can actually be used or not
		bool m_IsValid{true};
	};
} // namespace core::gfx
