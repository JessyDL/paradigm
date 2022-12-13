#pragma once
#include "fwd/resource/resource.hpp"
#include "gfx/types.hpp"
#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/meta.hpp"
#include "psl/serialization/serializer.hpp"

namespace core::meta
{
class shader;
}

namespace psl::meta
{
class library;
}

namespace core::data
{
/// \brief Describes a collection of resources that can be used to initialize a core::ivk::material_t
///
/// Material data describes a collection of textures, buffers, shaders, override parameters for these shaders,
/// blend operations (such as opaque or transparent), render order offsets, and generic options of how this material
/// *should* be rendered. It also contains facilities to set default values on buffers if needed.
class material_t final
{
	friend class psl::serialization::accessor;

  public:
	/// \brief special identifier to mark buffers that are instanciable data
	/// \details this type of buffer has data that is unique per instanciated material
	static constexpr psl::string_view MATERIAL_DATA = "MaterialData";

	/// \brief describes the blend operation (source/destination) per color component in the render operation.
	class blendstate
	{
		friend class psl::serialization::accessor;

	  public:
		static const blendstate opaque(uint32_t binding) { return blendstate(binding); }
		static const blendstate transparent(uint32_t binding)
		{
			using namespace core::gfx;
			return blendstate(true,
							  binding,
							  blend_factor::source_alpha,
							  blend_factor::one_minus_source_alpha,
							  blend_op::add,
							  blend_factor::one,
							  blend_factor::zero,
							  blend_op::add);
		}
		static const blendstate pre_multiplied_transparent(uint32_t binding)
		{
			using namespace core::gfx;
			return blendstate(true,
							  binding,
							  blend_factor::one,
							  blend_factor::one_minus_source_alpha,
							  blend_op::add,
							  blend_factor::one,
							  blend_factor::zero,
							  blend_op::add);
		}
		static const blendstate additive(uint32_t binding)
		{
			using namespace core::gfx;
			return blendstate(true,
							  binding,
							  blend_factor::one,
							  blend_factor::one,
							  blend_op::add,
							  blend_factor::one,
							  blend_factor::zero,
							  blend_op::add);
		}
		static const blendstate soft_additive(uint32_t binding)
		{
			using namespace core::gfx;
			return blendstate(true,
							  binding,
							  blend_factor::one_minus_destination_color,
							  blend_factor::one,
							  blend_op::add,
							  blend_factor::one,
							  blend_factor::zero,
							  blend_op::add);
		}
		static const blendstate multiplicative(uint32_t binding)
		{
			using namespace core::gfx;
			return blendstate(true,
							  binding,
							  blend_factor::dst_color,
							  blend_factor::zero,
							  blend_op::add,
							  blend_factor::one,
							  blend_factor::zero,
							  blend_op::add);
		}
		static const blendstate double_multiplicative(uint32_t binding)
		{
			using namespace core::gfx;
			return blendstate(true,
							  binding,
							  blend_factor::dst_color,
							  blend_factor::source_color,
							  blend_op::add,
							  blend_factor::one,
							  blend_factor::zero,
							  blend_op::add);
		}
		/// \param[in] enabled is the blendstate active (true) or not (false).
		/// \param[in] binding the binding location of the blend state.
		/// \param[in] srcColorBlend the operation to apply to the RGB components when loading.
		/// \param[in] dstColorBlend the operation to apply to the RGB components with our newly created color data.
		/// \param[in] colorBlendOp the blend operation to apply to the RGB components.
		/// \param[in] srcAlphaBlend the operation to apply to the A component when loading.
		/// \param[in] dstAlphaBlend the operation to apply to the A component with our newly created alpha data.
		/// \param[in] alphaBlendOp the blend operation to apply to the A component.
		/// \param[in] colorFlags the color component masking flags to use.
		blendstate(bool enabled,
				   uint32_t binding,
				   core::gfx::blend_factor srcColorBlend,
				   core::gfx::blend_factor dstColorBlend,
				   core::gfx::blend_op colorBlendOp,
				   core::gfx::blend_factor srcAlphaBlend,
				   core::gfx::blend_factor dstAlphaBlend,
				   core::gfx::blend_op alphaBlendOp,
				   core::gfx::component_bits colorFlags = (core::gfx::component_bits::r | core::gfx::component_bits::g |
														   core::gfx::component_bits::b |
														   core::gfx::component_bits::a)) :
			m_Enabled(enabled),
			m_Binding(binding), m_ColorBlendFactorSrc(srcColorBlend), m_ColorBlendFactorDst(dstColorBlend),
			m_AlphaBlendFactorSrc(srcAlphaBlend), m_AlphaBlendFactorDst(dstAlphaBlend),
			m_ColorComponents(colorFlags) {};
		blendstate(uint32_t binding) : m_Enabled(false), m_Binding(binding) {};
		blendstate() {};
		~blendstate()							 = default;
		blendstate(const blendstate&)			 = default;
		blendstate(blendstate&&)				 = default;
		blendstate& operator=(const blendstate&) = default;
		blendstate& operator=(blendstate&&)		 = default;

		bool enabled() const;
		uint32_t binding() const;
		core::gfx::blend_factor color_blend_src() const;
		core::gfx::blend_factor color_blend_dst() const;
		core::gfx::blend_op color_blend_op() const;
		core::gfx::blend_factor alpha_blend_src() const;
		core::gfx::blend_factor alpha_blend_dst() const;
		core::gfx::blend_op alpha_blend_op() const;
		core::gfx::component_bits color_components() const;

		void enabled(bool value);
		void binding(uint32_t value);
		void color_blend_src(core::gfx::blend_factor value);
		void color_blend_dst(core::gfx::blend_factor value);
		void color_blend_op(core::gfx::blend_op value);
		void alpha_blend_src(core::gfx::blend_factor value);
		void alpha_blend_dst(core::gfx::blend_factor value);
		void alpha_blend_op(core::gfx::blend_op value);
		void color_components(core::gfx::component_bits value);

	  private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_Binding << m_Enabled << m_ColorBlendFactorSrc << m_ColorBlendFactorDst << m_ColorBlendOp
					   << m_AlphaBlendFactorSrc << m_AlphaBlendFactorDst << m_AlphaBlendOp << m_ColorComponents;
		}
		static constexpr psl::string8::view serialization_name {"BLEND_STATE"};
		psl::serialization::property<"ENABLED", bool> m_Enabled {false};
		psl::serialization::property<"BINDING", uint32_t> m_Binding;
		psl::serialization::property<"COLOR_BLEND_SRC", core::gfx::blend_factor> m_ColorBlendFactorSrc {
		  core::gfx::blend_factor::one};
		psl::serialization::property<"COLOR_BLEND_DST", core::gfx::blend_factor> m_ColorBlendFactorDst {
		  core::gfx::blend_factor::zero};
		psl::serialization::property<"COLOR_BLEND_OP", core::gfx::blend_op> m_ColorBlendOp {core::gfx::blend_op::add};


		psl::serialization::property<"ALPHA_BLEND_SRC", core::gfx::blend_factor> m_AlphaBlendFactorSrc {
		  core::gfx::blend_factor::one};
		psl::serialization::property<"ALPHA_BLEND_DST", core::gfx::blend_factor> m_AlphaBlendFactorDst {
		  core::gfx::blend_factor::zero};
		psl::serialization::property<"ALPHA_BLEND_OP", core::gfx::blend_op> m_AlphaBlendOp {core::gfx::blend_op::add};

		psl::serialization::property<"COMPONENT_FLAGS", core::gfx::component_bits> m_ColorComponents {
		  core::gfx::component_bits::r | core::gfx::component_bits::g | core::gfx::component_bits::b |
		  core::gfx::component_bits::a};
	};

	class attribute
	{
		friend class psl::serialization::accessor;

	  public:
		uint32_t location() const noexcept;
		void location(uint32_t value) noexcept;

		/// \brief returns the UID of the resource attached to this attribute
		/// \warning can return an invalid UID in case the resource was not present in the meta library
		/// when this attribute was loaded, in that case use the tag to find it.
		const psl::UID& buffer() const noexcept;
		void buffer(psl::UID value) noexcept;

		psl::string_view tag() const noexcept;
		void tag(psl::string8_t value) noexcept;

		/// \warning this is only valid/used when the attribute is attached to a vertex shader
		const std::optional<core::gfx::vertex_input_rate>& input_rate() const noexcept;
		void input_rate(core::gfx::vertex_input_rate value) noexcept;

	  private:
		static constexpr psl::string8::view serialization_name {"ATTRIBUTE"};

		template <typename S>
		void serialize(S& s)
		{
			s << m_Location;

			if constexpr(psl::serialization::details::IsDecoder<S>)
			{
				psl::serialization::property<"INPUT_RATE", int32_t> input_rate {-1};
				psl::serialization::property<"TAG", psl::string8_t> tag;
				psl::serialization::property<"BUFFER", psl::UID> buffer;
				s << input_rate << tag << buffer;
				if(input_rate.value != -1) m_InputRate = (core::gfx::vertex_input_rate)input_rate.value;

				if(tag.value.size() > 0)
				{
					m_Tag = tag.value;
				}
				else
				{
					m_Buffer = buffer.value;
				}
			}
			else
			{
				if(m_InputRate)
				{
					psl::serialization::property<"INPUT_RATE", core::gfx::vertex_input_rate> input_rate {
					  m_InputRate.value()};
					s << input_rate;
				}
				if(m_Tag.size() > 0)
				{
					psl::serialization::property<"TAG", psl::string8_t> tag {m_Tag};
					s << tag;
				}
				else
				{
					psl::serialization::property<"BUFFER", psl::UID> buffer {m_Buffer};
					s << buffer;
				}
			}
		}

		psl::serialization::property<"LOCATION", uint32_t> m_Location;

		// if the attribute is in a vertex shader, then this will be set.
		std::optional<core::gfx::vertex_input_rate> m_InputRate;

		psl::UID m_Buffer;
		psl::string8_t m_Tag;
	};

	class binding
	{
		friend class psl::serialization::accessor;

	  public:
		binding()						   = default;
		~binding()						   = default;
		binding(const binding&)			   = default;
		binding(binding&&)				   = default;
		binding& operator=(const binding&) = default;
		binding& operator=(binding&&)	   = default;

		uint32_t binding_slot() const;
		core::gfx::binding_type descriptor() const;
		const psl::UID& texture() const;
		const psl::UID& sampler() const;
		const psl::UID& buffer() const;

		void binding_slot(uint32_t value);
		void descriptor(core::gfx::binding_type value);
		void texture(const psl::UID& value, psl::string_view tag = {});
		void sampler(const psl::UID& value, psl::string_view tag = {});
		void buffer(const psl::UID& value, psl::string_view tag = {});

	  private:
		template <typename S>
		void serialize(S& s)
		{
			s << m_Binding << m_Description;

			if constexpr(psl::serialization::details::IsDecoder<S>)
			{
				throw std::runtime_error("we need to solve the design issue of tagged resources");
				switch(m_Description.value)
				{
				case core::gfx::binding_type::combined_image_sampler:
				{
					psl::serialization::property<"TEXTURE", psl::string> uid {};
					s << uid;

					psl::serialization::property<"SAMPLER", psl::string> sampler {};
					s << sampler;
				}
				break;
				case core::gfx::binding_type::uniform_buffer:
				{
					psl::serialization::property<"UBO", psl::string> uid {};
					s << uid;
				}
				break;
				case core::gfx::binding_type::storage_buffer:
				{
					psl::serialization::property<"SSBO", psl::string> uid {};
					s << uid;
				}
				break;
				default:
					break;
				}
			}
			else
			{
				switch(m_Description.value)
				{
				case core::gfx::binding_type::combined_image_sampler:
				{
					if(m_UIDTag.size() > 0)
					{
						psl::serialization::property<"TEXTURE", psl::string> uid {m_UIDTag};
						s << uid;
					}
					else
					{
						psl::serialization::property<"TEXTURE", psl::UID> uid {m_UID};
						s << uid;
					}
					if(m_SamplerUIDTag.size() > 0)
					{
						psl::serialization::property<"SAMPLER", psl::string> sampler {m_SamplerUIDTag};
						s << sampler;
					}
					else
					{
						psl::serialization::property<"SAMPLER", psl::UID> sampler {m_SamplerUID};
						s << sampler;
					}
				}
				break;
				case core::gfx::binding_type::uniform_buffer:
				{
					if(m_BufferTag.size() > 0)
					{
						psl::serialization::property<"UBO", psl::string> uid {m_BufferTag};
						s << uid;
					}
					else
					{
						psl::serialization::property<"UBO", psl::UID> uid {m_Buffer};
						s << uid;
					}
				}
				break;
				case core::gfx::binding_type::storage_buffer:
				{
					if(m_BufferTag.size() > 0)
					{
						psl::serialization::property<"SSBO", psl::string> uid {m_BufferTag};
						s << uid;
					}
					else
					{
						psl::serialization::property<"SSBO", psl::UID> uid {m_Buffer};
						s << uid;
					}
				}
				break;
				default:
					break;
				}
			}
		}

		psl::serialization::property<"BINDING", uint32_t> m_Binding;	// the slot in the shader to bind to
		psl::serialization::property<"DESCRIPTOR", core::gfx::binding_type> m_Description;
		psl::UID m_UID;
		psl::UID m_SamplerUID;	  // in case of texture binding
		psl::UID m_Buffer;

		psl::string m_UIDTag;
		psl::string m_BufferTag;
		psl::string m_SamplerUIDTag;

		static constexpr psl::string8::view serialization_name {"MATERIAL_BINDING"};
	};

	class stage
	{
		friend class psl::serialization::accessor;

	  public:
		stage()						   = default;
		~stage()					   = default;
		stage(const stage&)			   = default;
		stage(stage&&)				   = default;
		stage& operator=(const stage&) = default;
		stage& operator=(stage&&)	   = default;

		gfx::shader_stage shader_stage() const noexcept;
		const psl::UID& shader() const noexcept;
		void shader(gfx::shader_stage stage, const psl::UID& value) noexcept;
		const psl::array<binding>& bindings() const noexcept;
		void bindings(psl::array<binding> value) noexcept;
		const psl::array<attribute>& attributes() const noexcept;
		void attributes(psl::array<attribute> value) noexcept;

		void set(const binding& value);
		void erase(const binding& value);

	  private:
		template <typename S>
		void serialize(S& s)
		{
			s << m_Stage << m_Shader << m_Attributes << m_Bindings;
		}
		psl::serialization::property<"STAGE", gfx::shader_stage> m_Stage;
		psl::serialization::property<"SHADER", psl::UID> m_Shader;
		psl::serialization::property<"ATTRIBUTES", psl::array<attribute>> m_Attributes;
		psl::serialization::property<"BINDINGS", psl::array<binding>> m_Bindings;
		static constexpr psl::string8::view serialization_name {"MATERIAL_STAGE"};
	};

	material_t(core::resource::cache_t& cache,
			   const core::resource::metadata& metaData,
			   psl::meta::file* metaFile) noexcept;
	// material_t(const material_t& other, const psl::UID& uid, core::resource::cache_t& cache);
	~material_t();

	material_t(const material_t&)			 = delete;
	material_t(material_t&&)				 = delete;
	material_t& operator=(const material_t&) = delete;
	material_t& operator=(material_t&&)		 = delete;

	const psl::array<stage>& stages() const;
	const psl::array<blendstate>& blend_states() const;
	const psl::array<psl::string8_t>& defines() const;
	core::gfx::cullmode cull_mode() const;
	core::gfx::compare_op depth_compare_op() const;
	uint32_t render_layer() const;
	bool depth_test() const;
	bool depth_write() const;
	bool wireframe() const;

	void stages(const psl::array<stage>& values);
	void blend_states(const psl::array<blendstate>& values);
	void defines(const psl::array<psl::string8_t>& values);
	void cull_mode(core::gfx::cullmode value);
	void depth_compare_op(core::gfx::compare_op value);
	void render_layer(uint32_t value);
	void depth_test(bool value);
	void depth_write(bool value);
	void wireframe(bool value);

	void set(const stage& value);
	void set(const blendstate& value);
	void define(psl::string8::view value);

	void erase(const stage& value);
	void erase(const blendstate& value);
	void undefine(psl::string8::view value);

	void from_shaders(const psl::meta::library& library, psl::array<core::meta::shader*> shaderMetas);

  private:
	template <typename S>
	void serialize(S& serializer)
	{
		serializer << m_Stage << m_Defines << m_Culling << m_DepthTest << m_DepthWrite << m_DepthCompareOp
				   << m_BlendStates << m_RenderLayer << m_Wireframe;
	}

	static constexpr psl::string8::view serialization_name {"MATERIAL"};

	psl::serialization::property<"STAGES", psl::array<stage>> m_Stage;
	psl::serialization::property<"BLEND_STATES", psl::array<blendstate>> m_BlendStates;
	psl::serialization::property<"DEFINES", psl::array<psl::string8_t>> m_Defines;
	psl::serialization::property<"CULLING", core::gfx::cullmode> m_Culling {core::gfx::cullmode::back};

	psl::serialization::property<"DEPTH_COMPARE", core::gfx::compare_op> m_DepthCompareOp {
	  core::gfx::compare_op::less_equal};
	psl::serialization::property<"RENDER_LAYER", uint32_t> m_RenderLayer {0};
	psl::serialization::property<"DEPTH_TEST", bool> m_DepthTest {true};
	psl::serialization::property<"DEPTH_WRITE", bool> m_DepthWrite {true};
	psl::serialization::property<"WIREFRAME_MODE", bool> m_Wireframe {false};
};
}	 // namespace core::data
