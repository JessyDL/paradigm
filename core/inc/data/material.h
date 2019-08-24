#pragma once
#include "serialization.h"
#include "vk/stdafx.h"
#include "meta.h"
#include "gfx/types.h"
#include "fwd/resource/resource.h"


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
	/// \brief Describes a collection of resources that can be used to initialize a core::ivk::material
	///
	/// Material data describes a collection of textures, buffers, shaders, override parameters for these shaders,
	/// blend operations (such as opaque or transparent), render order offsets, and generic options of how this material
	/// *should* be rendered. It also contains facilities to set default values on buffers if needed.
	class material final
	{
		friend class psl::serialization::accessor;

	  public:
		/// \brief describes the blend operation (source/destination) per color component in the render operation.
		class blendstate
		{
			friend class psl::serialization::accessor;

		  public:
			/// \param[in] enabled is the blendstate active (true) or not (false).
			/// \param[in] binding the binding location of the blend state.
			/// \param[in] srcColorBlend the operation to apply to the RGB components when loading.
			/// \param[in] dstColorBlend the operation to apply to the RGB components with our newly created color data.
			/// \param[in] colorBlendOp the blend operation to apply to the RGB components.
			/// \param[in] srcAlphaBlend the operation to apply to the A component when loading.
			/// \param[in] dstAlphaBlend the operation to apply to the A component with our newly created alpha data.
			/// \param[in] alphaBlendOp the blend operation to apply to the A component.
			/// \param[in] colorFlags the color component masking flags to use.
			blendstate(bool enabled, uint32_t binding, core::gfx::blend_factor srcColorBlend,
					   core::gfx::blend_factor dstColorBlend, core::gfx::blend_op colorBlendOp,
					   core::gfx::blend_factor srcAlphaBlend, core::gfx::blend_factor dstAlphaBlend,
					   core::gfx::blend_op alphaBlendOp,
					   core::gfx::component_bits colorFlags = {core::gfx::component_bits::r |
															   core::gfx::component_bits::g |
															   core::gfx::component_bits::b |
															   core::gfx::component_bits::a})
				: m_Enabled(enabled), m_Binding(binding), m_ColorBlendFactorSrc(srcColorBlend),
				  m_ColorBlendFactorDst(dstColorBlend), m_AlphaBlendFactorSrc(srcAlphaBlend),
				  m_AlphaBlendFactorDst(dstAlphaBlend), m_ColorComponents(colorFlags){};
			blendstate(){};
			~blendstate()				  = default;
			blendstate(const blendstate&) = default;
			blendstate(blendstate&&)	  = default;
			blendstate& operator=(const blendstate&) = default;
			blendstate& operator=(blendstate&&) = default;

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
				const uint32_t current_version = 1;
				psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
				serializer << version;

				switch(version)
				{
				case current_version:
					serializer << m_Binding << m_Enabled << m_ColorBlendFactorSrc << m_ColorBlendFactorDst
							   << m_ColorBlendOp << m_AlphaBlendFactorSrc << m_AlphaBlendFactorDst << m_AlphaBlendOp
							   << m_ColorComponents;
					break;
				case 0:
					psl::serialization::property<vk::BlendFactor, const_str("COLOR_BLEND_SRC", 15)> colorBlendFactorSrc{
						vk::BlendFactor::eOne};
					psl::serialization::property<vk::BlendFactor, const_str("COLOR_BLEND_DST", 15)> colorBlendFactorDst{
						vk::BlendFactor::eZero};
					psl::serialization::property<vk::BlendOp, const_str("COLOR_BLEND_OP", 14)> colorBlendOp{
						vk::BlendOp::eAdd};
					psl::serialization::property<vk::BlendFactor, const_str("ALPHA_BLEND_SRC", 15)> alphaBlendFactorSrc{
						vk::BlendFactor::eOne};
					psl::serialization::property<vk::BlendFactor, const_str("ALPHA_BLEND_DST", 15)> alphaBlendFactorDst{
						vk::BlendFactor::eZero};
					psl::serialization::property<vk::BlendOp, const_str("ALPHA_BLEND_OP", 14)> alphaBlendOp{
						vk::BlendOp::eAdd};
					psl::serialization::property<vk::ColorComponentFlags, const_str("COMPONENT_FLAGS", 15)>
						colorComponents{vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
										vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
					serializer << m_Binding << m_Enabled;
					serializer << colorBlendFactorSrc << colorBlendFactorDst << colorBlendOp << alphaBlendFactorSrc
							   << alphaBlendFactorDst << alphaBlendOp << colorComponents;

					m_ColorBlendFactorSrc.value = conversion::to_blend_factor(colorBlendFactorSrc.value);
					m_ColorBlendFactorDst.value = conversion::to_blend_factor(colorBlendFactorDst.value);
					m_ColorBlendOp.value		= conversion::to_blend_op(colorBlendOp.value);

					m_AlphaBlendFactorSrc.value = conversion::to_blend_factor(alphaBlendFactorSrc.value);
					m_AlphaBlendFactorDst.value = conversion::to_blend_factor(alphaBlendFactorDst.value);
					m_AlphaBlendOp.value		= conversion::to_blend_op(alphaBlendOp.value);

					m_ColorComponents.value = conversion::to_component_bits(colorComponents.value);
					break;
				}
			}
			static constexpr const char serialization_name[12]{"BLEND_STATE"};
			psl::serialization::property<bool, const_str("ENABLED", 7)> m_Enabled{false};
			psl::serialization::property<uint32_t, const_str("BINDING", 7)> m_Binding;
			psl::serialization::property<core::gfx::blend_factor, const_str("COLOR_BLEND_SRC", 15)>
				m_ColorBlendFactorSrc{core::gfx::blend_factor::one};
			psl::serialization::property<core::gfx::blend_factor, const_str("COLOR_BLEND_DST", 15)>
				m_ColorBlendFactorDst{core::gfx::blend_factor::zero};
			psl::serialization::property<core::gfx::blend_op, const_str("COLOR_BLEND_OP", 14)> m_ColorBlendOp{
				core::gfx::blend_op::add};


			psl::serialization::property<core::gfx::blend_factor, const_str("ALPHA_BLEND_SRC", 15)>
				m_AlphaBlendFactorSrc{core::gfx::blend_factor::one};
			psl::serialization::property<core::gfx::blend_factor, const_str("ALPHA_BLEND_DST", 15)>
				m_AlphaBlendFactorDst{core::gfx::blend_factor::zero};
			psl::serialization::property<core::gfx::blend_op, const_str("ALPHA_BLEND_OP", 14)> m_AlphaBlendOp{
				core::gfx::blend_op::add};

			psl::serialization::property<core::gfx::component_bits, const_str("COMPONENT_FLAGS", 15)> m_ColorComponents{
				core::gfx::component_bits::r | core::gfx::component_bits::g | core::gfx::component_bits::b |
				core::gfx::component_bits::a};
		};

		class binding
		{
			friend class psl::serialization::accessor;

		  public:
			binding()				= default;
			~binding()				= default;
			binding(const binding&) = default;
			binding(binding&&)		= default;
			binding& operator=(const binding&) = default;
			binding& operator=(binding&&) = default;

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
				s << m_Binding;

				const uint32_t current_version = 1;
				psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
				s << version;

				switch(version)
				{
				case current_version: s << m_Description; break;
				case 0:
					psl::serialization::property<vk::DescriptorType, const_str("DESCRIPTOR", 10)> description;
					s << description;

					m_Description.value = conversion::to_binding_type(description.value);
					break;
				}

				if constexpr(psl::serialization::details::is_decoder<S>::value)
				{
					throw std::runtime_error("we need to solve the design issue of tagged resources");
					switch(m_Description.value)
					{
					case core::gfx::binding_type::combined_image_sampler:
					{

						psl::serialization::property<psl::string, const_str("TEXTURE", 7)> uid{};
						s << uid;

						psl::serialization::property<psl::string, const_str("SAMPLER", 7)> sampler{};
						s << sampler;
					}
					break;
					case core::gfx::binding_type::uniform_buffer:
					{
						psl::serialization::property<psl::string, const_str("UBO", 3)> uid{};
						s << uid;
					}
					break;
					case core::gfx::binding_type::storage_buffer:
					{
						psl::serialization::property<psl::string, const_str("SSBO", 4)> uid{};
						s << uid;
					}
					break;
					default: break;
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
							psl::serialization::property<psl::string, const_str("TEXTURE", 7)> uid{m_UIDTag};
							s << uid;
						}
						else
						{
							psl::serialization::property<psl::UID, const_str("TEXTURE", 7)> uid{m_UID};
							s << uid;
						}
						if(m_SamplerUIDTag.size() > 0)
						{
							psl::serialization::property<psl::string, const_str("SAMPLER", 7)> sampler{m_SamplerUIDTag};
							s << sampler;
						}
						else
						{
							psl::serialization::property<psl::UID, const_str("SAMPLER", 7)> sampler{m_SamplerUID};
							s << sampler;
						}
					}
					break;
					case core::gfx::binding_type::uniform_buffer:
					{
						if(m_BufferTag.size() > 0)
						{
							psl::serialization::property<psl::string, const_str("UBO", 3)> uid{m_BufferTag};
							s << uid;
						}
						else
						{
							psl::serialization::property<psl::UID, const_str("UBO", 3)> uid{m_Buffer};
							s << uid;
						}
					}
					break;
					case core::gfx::binding_type::storage_buffer:
					{
						if(m_BufferTag.size() > 0)
						{
							psl::serialization::property<psl::string, const_str("SSBO", 4)> uid{m_BufferTag};
							s << uid;
						}
						else
						{
							psl::serialization::property<psl::UID, const_str("SSBO", 4)> uid{m_Buffer};
							s << uid;
						}
					}
					break;
					default: break;
					}
				}
			}

			psl::serialization::property<uint32_t, const_str("BINDING", 7)>
				m_Binding; // the slot in the shader to bind to
			psl::serialization::property<core::gfx::binding_type, const_str("DESCRIPTOR", 10)> m_Description;
			psl::UID m_UID;
			psl::UID m_SamplerUID; // in case of texture binding
			psl::UID m_Buffer;

			psl::string m_UIDTag;
			psl::string m_BufferTag;
			psl::string m_SamplerUIDTag;

			static constexpr const char serialization_name[17]{"MATERIAL_BINDING"};
		};

		class stage
		{
			friend class psl::serialization::accessor;

		  public:
			stage()				= default;
			~stage()			= default;
			stage(const stage&) = default;
			stage(stage&&)		= default;
			stage& operator=(const stage&) = default;
			stage& operator=(stage&&) = default;

			gfx::shader_stage shader_stage() const noexcept;
			const psl::UID& shader() const noexcept;
			const std::vector<binding>& bindings() const noexcept;

			void shader(gfx::shader_stage stage, const psl::UID& value) noexcept;
			void bindings(const std::vector<binding>& value);

			void set(const binding& value);
			void erase(const binding& value);

		  private:
			template <typename S>
			void serialize(S& s)
			{
				const uint32_t current_version = 1;
				psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
				s << version;

				switch(version)
				{
				case current_version: s << m_Stage << m_Shader << m_Bindings; break;
				case 0:
					psl::serialization::property<vk::ShaderStageFlags, const_str("STAGE", 5)> stage;
					s << stage;
					m_Stage.value = gfx::to_shader_stage(stage.value);
					s << m_Shader << m_Bindings;
				}
			}
			psl::serialization::property<gfx::shader_stage, const_str("STAGE", 5)> m_Stage;
			psl::serialization::property<psl::UID, const_str("SHADER", 6)> m_Shader;
			psl::serialization::property<std::vector<binding>, const_str("BINDINGS", 8)> m_Bindings;
			static constexpr const char serialization_name[15]{"MATERIAL_STAGE"};
		};

		material(core::resource::cache& cache, const core::resource::metadata& metaData,
				 psl::meta::file* metaFile) noexcept;
		// material(const material& other, const psl::UID& uid, core::resource::cache& cache);
		~material();

		material(const material&) = delete;
		material(material&&)	  = delete;
		material& operator=(const material&) = delete;
		material& operator=(material&&) = delete;

		const std::vector<stage>& stages() const;
		const std::vector<blendstate>& blend_states() const;
		const std::vector<psl::string8_t>& defines() const;
		core::gfx::cullmode cull_mode() const;
		core::gfx::compare_op depth_compare_op() const;
		uint32_t render_layer() const;
		bool depth_test() const;
		bool depth_write() const;
		bool wireframe() const;

		void stages(const std::vector<stage>& values);
		void blend_states(const std::vector<blendstate>& values);
		void defines(const std::vector<psl::string8_t>& values);
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

		void from_shaders(const psl::meta::library& library, std::vector<core::meta::shader*> shaderMetas);

	  private:
		template <typename S>
		void serialize(S& serializer)
		{
			const uint32_t current_version = 1;
			psl::serialization::property<uint32_t, const_str("VERSION", 7)> version{0};
			serializer << version;

			switch(version)
			{
			case current_version:
				serializer << m_Stage << m_Defines << m_Culling << m_DepthTest << m_DepthWrite << m_DepthCompareOp
						   << m_BlendStates << m_RenderLayer << m_Wireframe;
				break;
			case 0:
				break;

				psl::serialization::property<vk::CullModeFlagBits, const_str("CULLING", 7)> culling{
					vk::CullModeFlagBits::eBack};

				psl::serialization::property<vk::CompareOp, const_str("DEPTH_COMPARE", 13)> depthCompareOp{
					vk::CompareOp::eLessOrEqual};

				serializer << culling << depthCompareOp;

				m_Culling.value		   = conversion::to_cullmode(culling.value);
				m_DepthCompareOp.value = conversion::to_compare_op(depthCompareOp.value);
				serializer << m_Stage << m_Defines << m_DepthTest << m_DepthWrite << m_BlendStates << m_RenderLayer
						   << m_Wireframe;
			}
		}

		static constexpr const char serialization_name[9]{"MATERIAL"};

		psl::serialization::property<std::vector<stage>, const_str("STAGES", 6)> m_Stage;
		psl::serialization::property<std::vector<blendstate>, const_str("BLEND_STATES", 12)> m_BlendStates;
		psl::serialization::property<std::vector<psl::string8_t>, const_str("DEFINES", 7)> m_Defines;
		psl::serialization::property<core::gfx::cullmode, const_str("CULLING", 7)> m_Culling{core::gfx::cullmode::back};

		psl::serialization::property<core::gfx::compare_op, const_str("DEPTH_COMPARE", 13)> m_DepthCompareOp{
			core::gfx::compare_op::less_equal};
		psl::serialization::property<uint32_t, const_str("RENDER_LAYER", 12)> m_RenderLayer{0};
		psl::serialization::property<bool, const_str("DEPTH_TEST", 10)> m_DepthTest{true};
		psl::serialization::property<bool, const_str("DEPTH_WRITE", 11)> m_DepthWrite{true};
		psl::serialization::property<bool, const_str("WIREFRAME_MODE", 14)> m_Wireframe{false};
	};
} // namespace core::data
