#include "data/material.h"
#include "data/geometry.h"
#include "gfx/bundle.h"
#include "meta/shader.h"
#include "resource/resource.hpp"

using namespace psl;
using namespace core::data;

material_t::material_t(core::resource::cache_t& cache,
				   const core::resource::metadata& metaData,
				   psl::meta::file* metaFile) noexcept
{}
// material_t::material_t(const material_t& other, const UID& uid, core::resource::cache_t& cache)
//	: m_Stage(other.m_Stage), m_BlendStates(other.m_BlendStates), m_Defines(other.m_Defines),
//	m_Culling(other.m_Culling), m_DepthCompareOp(other.m_DepthCompareOp), m_DepthTest(other.m_DepthTest),
//	m_DepthWrite(other.m_DepthWrite), m_Wireframe(other.m_Wireframe)
//{
//
//};
material_t::~material_t() {}

// blendstate
bool material_t::blendstate::enabled() const { return m_Enabled.value; }
uint32_t material_t::blendstate::binding() const { return m_Binding.value; }
core::gfx::blend_factor material_t::blendstate::color_blend_src() const { return m_ColorBlendFactorSrc.value; }
core::gfx::blend_factor material_t::blendstate::color_blend_dst() const { return m_ColorBlendFactorDst.value; }
core::gfx::blend_op material_t::blendstate::color_blend_op() const { return m_ColorBlendOp.value; }
core::gfx::blend_factor material_t::blendstate::alpha_blend_src() const { return m_AlphaBlendFactorSrc.value; }
core::gfx::blend_factor material_t::blendstate::alpha_blend_dst() const { return m_AlphaBlendFactorDst.value; }
core::gfx::blend_op material_t::blendstate::alpha_blend_op() const { return m_AlphaBlendOp.value; }
core::gfx::component_bits material_t::blendstate::color_components() const { return m_ColorComponents.value; }

void material_t::blendstate::enabled(bool value) { m_Enabled.value = value; }
void material_t::blendstate::binding(uint32_t value) { m_Binding.value = value; }
void material_t::blendstate::color_blend_src(core::gfx::blend_factor value) { m_ColorBlendFactorSrc.value = value; }
void material_t::blendstate::color_blend_dst(core::gfx::blend_factor value) { m_ColorBlendFactorDst.value = value; }
void material_t::blendstate::color_blend_op(core::gfx::blend_op value) { m_ColorBlendOp.value = value; }
void material_t::blendstate::alpha_blend_src(core::gfx::blend_factor value) { m_AlphaBlendFactorSrc.value = value; }
void material_t::blendstate::alpha_blend_dst(core::gfx::blend_factor value) { m_AlphaBlendFactorDst.value = value; }
void material_t::blendstate::alpha_blend_op(core::gfx::blend_op value) { m_AlphaBlendOp.value = value; }
void material_t::blendstate::color_components(core::gfx::component_bits value) { m_ColorComponents.value = value; }

// attribute
uint32_t material_t::attribute::location() const noexcept { return m_Location.value; }
void material_t::attribute::location(uint32_t value) noexcept { m_Location.value = value; }

const psl::UID& material_t::attribute::buffer() const noexcept { return m_Buffer; }
void material_t::attribute::buffer(psl::UID value) noexcept { m_Buffer = std::move(value); }

psl::string_view material_t::attribute::tag() const noexcept { return m_Tag; }
void material_t::attribute::tag(psl::string8_t value) noexcept { m_Tag = std::move(value); }

const std::optional<core::gfx::vertex_input_rate>& material_t::attribute::input_rate() const noexcept
{
	return m_InputRate;
}
void material_t::attribute::input_rate(core::gfx::vertex_input_rate value) noexcept { m_InputRate = value; }

// binding
uint32_t material_t::binding::binding_slot() const { return m_Binding.value; }
core::gfx::binding_type material_t::binding::descriptor() const { return m_Description.value; }
const UID& material_t::binding::texture() const { return m_UID; }
const UID& material_t::binding::sampler() const { return m_SamplerUID; }
const UID& material_t::binding::buffer() const { return m_Buffer; }

void material_t::binding::binding_slot(uint32_t value) { m_Binding.value = value; }
void material_t::binding::descriptor(core::gfx::binding_type value) { m_Description.value = value; }
void material_t::binding::texture(const UID& value, psl::string_view tag)
{
	m_UID	 = value;
	m_UIDTag = tag;
}
void material_t::binding::sampler(const UID& value, psl::string_view tag)
{
	m_SamplerUID	= value;
	m_SamplerUIDTag = tag;
}
void material_t::binding::buffer(const UID& value, psl::string_view tag)
{
	m_Buffer	= value;
	m_BufferTag = tag;
}
// stage
core::gfx::shader_stage material_t::stage::shader_stage() const noexcept { return m_Stage.value; }
const UID& material_t::stage::shader() const noexcept { return m_Shader.value; }
const psl::array<material_t::binding>& material_t::stage::bindings() const noexcept { return m_Bindings.value; }

void material_t::stage::shader(core::gfx::shader_stage stage, const UID& value) noexcept
{
	m_Stage.value  = stage;
	m_Shader.value = value;
}
void material_t::stage::bindings(psl::array<binding> value) noexcept { m_Bindings.value = std::move(value); }

const psl::array<material_t::attribute>& material_t::stage::attributes() const noexcept { return m_Attributes.value; }
void material_t::stage::attributes(psl::array<material_t::attribute> value) noexcept
{
	m_Attributes.value = std::move(value);
}

void material_t::stage::set(const binding& value)
{
	auto it = std::find_if(std::begin(m_Bindings.value), std::end(m_Bindings.value), [&value](const binding& element) {
		return element.binding_slot() == value.binding_slot();
	});
	if(it == std::end(m_Bindings.value))
		m_Bindings.value.emplace_back(value);
	else
		*it = value;
}
void material_t::stage::erase(const binding& value)
{
	auto it = std::find_if(std::begin(m_Bindings.value), std::end(m_Bindings.value), [&value](const binding& element) {
		return element.binding_slot() == value.binding_slot();
	});
	if(it != std::end(m_Bindings.value)) m_Bindings.value.erase(it);
}
// material
const psl::array<material_t::stage>& material_t::stages() const { return m_Stage.value; }
const psl::array<material_t::blendstate>& material_t::blend_states() const { return m_BlendStates.value; }
const psl::array<psl::string8_t>& material_t::defines() const { return m_Defines.value; }
core::gfx::cullmode material_t::cull_mode() const { return m_Culling.value; }
core::gfx::compare_op material_t::depth_compare_op() const { return m_DepthCompareOp.value; }
uint32_t material_t::render_layer() const { return m_RenderLayer.value; }
bool material_t::depth_test() const { return m_DepthTest.value; }
bool material_t::depth_write() const { return m_DepthWrite.value; }
bool material_t::wireframe() const { return m_Wireframe.value; }

void material_t::stages(const psl::array<stage>& values) { m_Stage.value = values; }
void material_t::blend_states(const psl::array<blendstate>& values) { m_BlendStates.value = values; }
void material_t::defines(const psl::array<psl::string8_t>& values) { m_Defines.value = values; }
void material_t::cull_mode(core::gfx::cullmode value) { m_Culling.value = value; }
void material_t::depth_compare_op(core::gfx::compare_op value) { m_DepthCompareOp.value = value; }
void material_t::render_layer(uint32_t value) { m_RenderLayer.value = value; }
void material_t::depth_test(bool value) { m_DepthTest.value = value; }
void material_t::depth_write(bool value) { m_DepthWrite.value = value; }
void material_t::wireframe(bool value) { m_Wireframe.value = value; }


void material_t::set(const stage& value)
{
	auto it = std::find_if(std::begin(m_Stage.value), std::end(m_Stage.value), [&value](const stage& element) {
		return element.shader_stage() == value.shader_stage();
	});
	if(it == std::end(m_Stage.value))
		m_Stage.value.emplace_back(value);
	else
		*it = value;
}
void material_t::set(const blendstate& value)
{
	auto it = std::find_if(std::begin(m_BlendStates.value),
						   std::end(m_BlendStates.value),
						   [&value](const blendstate& element) { return element.binding() == value.binding(); });
	if(it == std::end(m_BlendStates.value))
		m_BlendStates.value.emplace_back(value);
	else
		*it = value;
}
void material_t::define(psl::string8::view value)
{
	auto it = std::find(std::begin(m_Defines.value), std::end(m_Defines.value), value);
	if(it == std::end(m_Defines.value)) m_Defines.value.emplace_back(value);
}

void material_t::erase(const stage& value)
{
	auto it = std::find_if(std::begin(m_Stage.value), std::end(m_Stage.value), [&value](const stage& element) {
		return element.shader_stage() == value.shader_stage();
	});
	if(it != std::end(m_Stage.value)) m_Stage.value.erase(it);
}
void material_t::erase(const blendstate& value)
{
	auto it = std::find_if(std::begin(m_BlendStates.value),
						   std::end(m_BlendStates.value),
						   [&value](const blendstate& element) { return element.binding() == value.binding(); });
	if(it != std::end(m_BlendStates.value)) m_BlendStates.value.erase(it);
}
void material_t::undefine(psl::string8::view value)
{
	auto it = std::find(std::begin(m_Defines.value), std::end(m_Defines.value), value);
	if(it == std::end(m_Defines.value)) m_Defines.value.erase(it);
}

void material_t::from_shaders(const psl::meta::library& library, psl::array<core::meta::shader*> shaderMetas)
{
	psl::array<stage>& stages = m_Stage.value;
	for(auto shader : shaderMetas)
	{
		auto it = std::find_if(std::begin(stages), std::end(stages), [&shader](const stage& stage) {
			return stage.shader_stage() == shader->stage();
		});

		auto& stage = ((it != std::end(stages)) ? *it : stages.emplace_back());

		stage.shader(shader->stage(), shader->ID());

		// auto& vBindings = shader->vertex_bindings();
		// for(auto i = 0u; i < vBindings.size(); ++i)
		//{
		//	binding& binding = bindings.emplace_back();
		//	binding.binding_slot(vBindings[i].binding_slot());
		//	if(auto uid = UID::convert(vBindings[i].buffer()); uid)
		//	{
		//		binding.buffer(uid);
		//		continue;
		//	}
		//	auto res =  library.find(vBindings[i].buffer());
		//	if(res)
		//		binding.buffer(res.value());
		//	//else
		//	//	core::data::log->warn("could not resolve the UID {0}. it was neither a tag or a valid UID.",
		// vBindings[i].buffer());

		//}

		psl::string_view instance_designator = "INSTANCE_";
		psl::array<attribute> attributes;

		using constants = core::data::geometry_t::constants;

		for(const auto& input : shader->inputs())
		{
			attribute& attribute = attributes.emplace_back();
			attribute.location(input.location());
			if(shader->stage() == core::gfx::shader_stage::vertex)
			{
				attribute.input_rate(core::gfx::vertex_input_rate::vertex);

				// todo we should figure out a way to configure these in a clean manner
				if(input.name() == "iPos" || input.name() == constants::POSITION)
					attribute.tag(psl::string {core::data::geometry_t::constants::POSITION});
				else if(input.name() == "iNorm" || input.name() == constants::NORMAL)
					attribute.tag(psl::string {core::data::geometry_t::constants::NORMAL});
				else if(input.name() == "iCol" || input.name() == constants::COLOR)
					attribute.tag(psl::string {core::data::geometry_t::constants::COLOR});
				else if(input.name() == "iTan" || input.name() == constants::TANGENT)
					attribute.tag(psl::string {core::data::geometry_t::constants::TANGENT});
				else if(input.name() == "iBiTan" || input.name() == constants::BITANGENT)
					attribute.tag(psl::string {core::data::geometry_t::constants::BITANGENT});
				else if(input.name() == "iTex" || input.name() == constants::TEX)
					attribute.tag(psl::string {core::data::geometry_t::constants::TEX});
				else if(input.name() == core::gfx::constants::INSTANCE_MODELMATRIX ||
						input.name() == core::gfx::constants::INSTANCE_LEGACY_MODELMATRIX)
				{
					attribute.tag(psl::string {core::gfx::constants::INSTANCE_MODELMATRIX});
					attribute.input_rate(core::gfx::vertex_input_rate::instance);
				}

				if(input.name().size() >= instance_designator.size() &&
				   input.name().substr(0, instance_designator.size()) == instance_designator)
				{
					attribute.tag(psl::string(input.name()));
					attribute.input_rate(core::gfx::vertex_input_rate::instance);
				}
			}
		}
		stage.attributes(std::move(attributes));
		psl::array<binding> bindings;
		for(const auto& descr : shader->descriptors())
		{
			binding& binding = bindings.emplace_back();
			binding.binding_slot(descr.binding());
			binding.descriptor(descr.type());
			switch(binding.descriptor())
			{
			case core::gfx::binding_type::storage_buffer:
			case core::gfx::binding_type::storage_buffer_dynamic:
			case core::gfx::binding_type::uniform_buffer:
			case core::gfx::binding_type::uniform_buffer_dynamic:
				if(auto uid = UID::from_string(psl::string(descr.name())); uid)
				{
					binding.buffer(uid);
				}
				else
				{
					auto res = library.find(descr.name());
					if(res) binding.buffer(res.value(), descr.name());
				}
				break;
			case core::gfx::binding_type::combined_image_sampler:
				break;

			default:
			{
			}
			}
		}
		stage.bindings(bindings);
	}
}