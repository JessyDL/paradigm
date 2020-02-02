#pragma once
#include "psl/serialization.h"
#include "psl/meta.h"
#include "psl/array.h"
#include "psl/array_view.h"
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
	template <typename T, char... Char>
	using sprop = psl::serialization::property<T, Char...>;
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
			static const blendstate opaque(uint32_t binding) { return blendstate(binding); }
			static const blendstate transparent(uint32_t binding)
			{
				using namespace core::gfx;
				return blendstate(true, binding, blend_factor::source_alpha, blend_factor::one_minus_source_alpha,
								  blend_op::add, blend_factor::one, blend_factor::zero, blend_op::add);
			}
			static const blendstate pre_multiplied_transparent(uint32_t binding)
			{
				using namespace core::gfx;
				return blendstate(true, binding, blend_factor::one, blend_factor::one_minus_source_alpha, blend_op::add,
								  blend_factor::one, blend_factor::zero, blend_op::add);
			}
			static const blendstate additive(uint32_t binding)
			{
				using namespace core::gfx;
				return blendstate(true, binding, blend_factor::one, blend_factor::one, blend_op::add, blend_factor::one,
								  blend_factor::zero, blend_op::add);
			}
			static const blendstate soft_additive(uint32_t binding)
			{
				using namespace core::gfx;
				return blendstate(true, binding, blend_factor::one_minus_destination_color, blend_factor::one,
								  blend_op::add, blend_factor::one, blend_factor::zero, blend_op::add);
			}
			static const blendstate multiplicative(uint32_t binding)
			{
				using namespace core::gfx;
				return blendstate(true, binding, blend_factor::dst_color, blend_factor::zero, blend_op::add,
								  blend_factor::one, blend_factor::zero, blend_op::add);
			}
			static const blendstate double_multiplicative(uint32_t binding)
			{
				using namespace core::gfx;
				return blendstate(true, binding, blend_factor::dst_color, blend_factor::source_color, blend_op::add,
								  blend_factor::one, blend_factor::zero, blend_op::add);
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
			blendstate(bool enabled, uint32_t binding, core::gfx::blend_factor srcColorBlend,
					   core::gfx::blend_factor dstColorBlend, core::gfx::blend_op colorBlendOp,
					   core::gfx::blend_factor srcAlphaBlend, core::gfx::blend_factor dstAlphaBlend,
					   core::gfx::blend_op alphaBlendOp,
					   core::gfx::component_bits colorFlags = (core::gfx::component_bits::r |
															   core::gfx::component_bits::g |
															   core::gfx::component_bits::b |
															   core::gfx::component_bits::a))
				: m_Enabled(enabled), m_Binding(binding), m_ColorBlendFactorSrc(srcColorBlend),
				  m_ColorBlendFactorDst(dstColorBlend), m_AlphaBlendFactorSrc(srcAlphaBlend),
				  m_AlphaBlendFactorDst(dstAlphaBlend), m_ColorComponents(colorFlags){};
			blendstate(uint32_t binding) : m_Enabled(false), m_Binding(binding){};
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
				serializer << m_Binding << m_Enabled << m_ColorBlendFactorSrc << m_ColorBlendFactorDst << m_ColorBlendOp
						   << m_AlphaBlendFactorSrc << m_AlphaBlendFactorDst << m_AlphaBlendOp << m_ColorComponents;
			}
			static constexpr const char serialization_name[12]{"BLEND_STATE"};
			sprop<bool, const_str("ENABLED", 7)> m_Enabled{false};
			sprop<uint32_t, const_str("BINDING", 7)> m_Binding;
			sprop<core::gfx::blend_factor, const_str("COLOR_BLEND_SRC", 15)> m_ColorBlendFactorSrc{
				core::gfx::blend_factor::one};
			sprop<core::gfx::blend_factor, const_str("COLOR_BLEND_DST", 15)> m_ColorBlendFactorDst{
				core::gfx::blend_factor::zero};
			sprop<core::gfx::blend_op, const_str("COLOR_BLEND_OP", 14)> m_ColorBlendOp{core::gfx::blend_op::add};


			sprop<core::gfx::blend_factor, const_str("ALPHA_BLEND_SRC", 15)> m_AlphaBlendFactorSrc{
				core::gfx::blend_factor::one};
			sprop<core::gfx::blend_factor, const_str("ALPHA_BLEND_DST", 15)> m_AlphaBlendFactorDst{
				core::gfx::blend_factor::zero};
			sprop<core::gfx::blend_op, const_str("ALPHA_BLEND_OP", 14)> m_AlphaBlendOp{core::gfx::blend_op::add};

			sprop<core::gfx::component_bits, const_str("COMPONENT_FLAGS", 15)> m_ColorComponents{
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
			static constexpr const char serialization_name[10]{"ATTRIBUTE"};

			template <typename S>
			void serialize(S& s)
			{
				s << m_Location;

				if constexpr(psl::serialization::details::is_decoder<S>::value)
				{
					sprop<int32_t, const_str("INPUT_RATE", 10)> input_rate{-1};
					sprop<psl::string8_t, const_str("TAG", 3)> tag;
					sprop<psl::UID, const_str("BUFFER", 6)> buffer;
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
						sprop<core::gfx::vertex_input_rate, const_str("INPUT_RATE", 10)> input_rate{
							m_InputRate.value()};
						s << input_rate;
					}
					if(m_Tag.size() > 0)
					{
						sprop<psl::string8_t, const_str("TAG", 3)> tag{m_Tag};
						s << tag;
					}
					else
					{
						sprop<psl::UID, const_str("BUFFER", 6)> buffer{m_Buffer};
						s << buffer;
					}
				}
			}

			sprop<uint32_t, const_str("LOCATION", 8)> m_Location;

			// if the attribute is in a vertex shader, then this will be set.
			std::optional<core::gfx::vertex_input_rate> m_InputRate;

			psl::UID m_Buffer;
			psl::string8_t m_Tag;
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
				s << m_Binding << m_Description;

				if constexpr(psl::serialization::details::is_decoder<S>::value)
				{
					throw std::runtime_error("we need to solve the design issue of tagged resources");
					switch(m_Description.value)
					{
					case core::gfx::binding_type::combined_image_sampler:
					{

						sprop<psl::string, const_str("TEXTURE", 7)> uid{};
						s << uid;

						sprop<psl::string, const_str("SAMPLER", 7)> sampler{};
						s << sampler;
					}
					break;
					case core::gfx::binding_type::uniform_buffer:
					{
						sprop<psl::string, const_str("UBO", 3)> uid{};
						s << uid;
					}
					break;
					case core::gfx::binding_type::storage_buffer:
					{
						sprop<psl::string, const_str("SSBO", 4)> uid{};
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
							sprop<psl::string, const_str("TEXTURE", 7)> uid{m_UIDTag};
							s << uid;
						}
						else
						{
							sprop<psl::UID, const_str("TEXTURE", 7)> uid{m_UID};
							s << uid;
						}
						if(m_SamplerUIDTag.size() > 0)
						{
							sprop<psl::string, const_str("SAMPLER", 7)> sampler{m_SamplerUIDTag};
							s << sampler;
						}
						else
						{
							sprop<psl::UID, const_str("SAMPLER", 7)> sampler{m_SamplerUID};
							s << sampler;
						}
					}
					break;
					case core::gfx::binding_type::uniform_buffer:
					{
						if(m_BufferTag.size() > 0)
						{
							sprop<psl::string, const_str("UBO", 3)> uid{m_BufferTag};
							s << uid;
						}
						else
						{
							sprop<psl::UID, const_str("UBO", 3)> uid{m_Buffer};
							s << uid;
						}
					}
					break;
					case core::gfx::binding_type::storage_buffer:
					{
						if(m_BufferTag.size() > 0)
						{
							sprop<psl::string, const_str("SSBO", 4)> uid{m_BufferTag};
							s << uid;
						}
						else
						{
							sprop<psl::UID, const_str("SSBO", 4)> uid{m_Buffer};
							s << uid;
						}
					}
					break;
					default: break;
					}
				}
			}

			sprop<uint32_t, const_str("BINDING", 7)> m_Binding; // the slot in the shader to bind to
			sprop<core::gfx::binding_type, const_str("DESCRIPTOR", 10)> m_Description;
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
			sprop<gfx::shader_stage, const_str("STAGE", 5)> m_Stage;
			sprop<psl::UID, const_str("SHADER", 6)> m_Shader;
			sprop<psl::array<attribute>, const_str("ATTRIBUTES", 10)> m_Attributes;
			sprop<psl::array<binding>, const_str("BINDINGS", 8)> m_Bindings;
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

		static constexpr const char serialization_name[9]{"MATERIAL"};

		sprop<psl::array<stage>, const_str("STAGES", 6)> m_Stage;
		sprop<psl::array<blendstate>, const_str("BLEND_STATES", 12)> m_BlendStates;
		sprop<psl::array<psl::string8_t>, const_str("DEFINES", 7)> m_Defines;
		sprop<core::gfx::cullmode, const_str("CULLING", 7)> m_Culling{core::gfx::cullmode::back};

		sprop<core::gfx::compare_op, const_str("DEPTH_COMPARE", 13)> m_DepthCompareOp{
			core::gfx::compare_op::less_equal};
		sprop<uint32_t, const_str("RENDER_LAYER", 12)> m_RenderLayer{0};
		sprop<bool, const_str("DEPTH_TEST", 10)> m_DepthTest{true};
		sprop<bool, const_str("DEPTH_WRITE", 11)> m_DepthWrite{true};
		sprop<bool, const_str("WIREFRAME_MODE", 14)> m_Wireframe{false};
	};
} // namespace core::data
