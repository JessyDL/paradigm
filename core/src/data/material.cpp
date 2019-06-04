#include "data/material.h"
#include "meta/shader.h"
#include "systems/resource.h"

using namespace psl;
using namespace core::data;

material::material(const UID& uid, core::resource::cache& cache) {}
material::material(const material& other, const UID& uid, core::resource::cache& cache) 
	: m_Stage(other.m_Stage), m_BlendStates(other.m_BlendStates), m_Defines(other.m_Defines),
	m_Culling(other.m_Culling), m_DepthCompareOp(other.m_DepthCompareOp), m_DepthTest(other.m_DepthTest),
	m_DepthWrite(other.m_DepthWrite), m_Wireframe(other.m_Wireframe)
{

};
material ::~material() {}

// blendstate
bool material::blendstate::enabled() const { return m_Enabled.value; }
uint32_t material::blendstate::binding() const { return m_Binding.value; }
vk::BlendFactor material::blendstate::color_blend_src() const { return m_ColorBlendFactorSrc.value; }
vk::BlendFactor material::blendstate::color_blend_dst() const { return m_ColorBlendFactorDst.value; }
vk::BlendOp material::blendstate::color_blend_op() const { return m_ColorBlendOp.value; }
vk::BlendFactor material::blendstate::alpha_blend_src() const { return m_AlphaBlendFactorSrc.value; }
vk::BlendFactor material::blendstate::alpha_blend_dst() const { return m_AlphaBlendFactorDst.value; }
vk::BlendOp material::blendstate::alpha_blend_op() const { return m_AlphaBlendOp.value; }
vk::ColorComponentFlags material::blendstate::color_components() const { return m_ColorComponents.value; }

void material::blendstate::enabled(bool value) { m_Enabled.value = value; }
void material::blendstate::binding(uint32_t value) { m_Binding.value = value; }
void material::blendstate::color_blend_src(vk::BlendFactor value) { m_ColorBlendFactorSrc.value = value; }
void material::blendstate::color_blend_dst(vk::BlendFactor value) { m_ColorBlendFactorDst.value = value; }
void material::blendstate::color_blend_op(vk::BlendOp value) { m_ColorBlendOp.value = value; }
void material::blendstate::alpha_blend_src(vk::BlendFactor value) { m_AlphaBlendFactorSrc.value = value; }
void material::blendstate::alpha_blend_dst(vk::BlendFactor value) { m_AlphaBlendFactorDst.value = value; }
void material::blendstate::alpha_blend_op(vk::BlendOp value) { m_AlphaBlendOp.value = value; }
void material::blendstate::color_components(vk::ColorComponentFlags value) { m_ColorComponents.value = value; }
// binding
uint32_t material::binding::binding_slot() const { return m_Binding.value; }
vk::DescriptorType material::binding::descriptor() const { return m_Description.value; }
const UID& material::binding::texture() const { return m_UID; }
const UID& material::binding::sampler() const { return m_SamplerUID; }
const UID& material::binding::buffer() const { return m_Buffer; }

void material::binding::binding_slot(uint32_t value) { m_Binding.value = value; }
void material::binding::descriptor(vk::DescriptorType value) { m_Description.value = value; }
void material::binding::texture(const UID& value, psl::string_view tag) { m_UID = value; m_UIDTag = tag; }
void material::binding::sampler(const UID& value, psl::string_view tag) { m_SamplerUID = value; m_SamplerUIDTag = tag; }
void material::binding::buffer(const UID& value, psl::string_view tag) { m_Buffer = value; m_BufferTag = tag;}
// stage
const vk::ShaderStageFlags material::stage::shader_stage() const { return m_Stage.value; }
const UID& material::stage::shader() const { return m_Shader.value; }
const std::vector<material::binding>& material::stage::bindings() const { return m_Bindings.value; }

void material::stage::shader(vk::ShaderStageFlags stage, const UID& value)
{
	m_Stage.value  = stage;
	m_Shader.value = value;
}
void material::stage::bindings(const std::vector<binding>& value) { m_Bindings.value = value; }

void material::stage::set(const binding& value)
{
	auto it = std::find_if(std::begin(m_Bindings.value), std::end(m_Bindings.value),
						   [&value](const binding& element) { return element.binding_slot() == value.binding_slot(); });
	if(it == std::end(m_Bindings.value))
		m_Bindings.value.emplace_back(value);
	else
		*it = value;
}
void material::stage::erase(const binding& value)
{
	auto it = std::find_if(std::begin(m_Bindings.value), std::end(m_Bindings.value),
						   [&value](const binding& element) { return element.binding_slot() == value.binding_slot(); });
	if(it != std::end(m_Bindings.value)) m_Bindings.value.erase(it);
}
// material
const std::vector<material::stage>& material::stages() const { return m_Stage.value; }
const std::vector<material::blendstate>& material::blend_states() const { return m_BlendStates.value; }
const std::vector<psl::string8_t>& material::defines() const { return m_Defines.value; }
vk::CullModeFlagBits material::cull_mode() const { return m_Culling.value; }
vk::CompareOp material::depth_compare_op() const { return m_DepthCompareOp.value; }
uint32_t material::render_layer() const { return m_RenderLayer.value; }
bool material::depth_test() const { return m_DepthTest.value; }
bool material::depth_write() const { return m_DepthWrite.value; }
bool material::wireframe() const { return m_Wireframe.value; }

void material::stages(const std::vector<stage>& values) { m_Stage.value = values; }
void material::blend_states(const std::vector<blendstate>& values) { m_BlendStates.value = values; }
void material::defines(const std::vector<psl::string8_t>& values) { m_Defines.value = values; }
void material::cull_mode(vk::CullModeFlagBits value) { m_Culling.value = value; }
void material::depth_compare_op(vk::CompareOp value) { m_DepthCompareOp.value = value; }
void material::render_layer(uint32_t value) { m_RenderLayer.value = value; }
void material::depth_test(bool value) { m_DepthTest.value = value; }
void material::depth_write(bool value) { m_DepthWrite.value = value; }
void material::wireframe(bool value) { m_Wireframe.value = value; }


void material::set(const stage& value)
{
	auto it = std::find_if(std::begin(m_Stage.value), std::end(m_Stage.value),
						   [&value](const stage& element) { return element.shader_stage() == value.shader_stage(); });
	if(it == std::end(m_Stage.value))
		m_Stage.value.emplace_back(value);
	else
		*it = value;
}
void material::set(const blendstate& value)
{
	auto it = std::find_if(std::begin(m_BlendStates.value), std::end(m_BlendStates.value),
						   [&value](const blendstate& element) { return element.binding() == value.binding(); });
	if(it == std::end(m_BlendStates.value))
		m_BlendStates.value.emplace_back(value);
	else
		*it = value;
}
void material::define(psl::string8::view value)
{
	auto it = std::find(std::begin(m_Defines.value), std::end(m_Defines.value), value);
	if(it == std::end(m_Defines.value)) m_Defines.value.emplace_back(value);
}

void material::erase(const stage& value)
{
	auto it = std::find_if(std::begin(m_Stage.value), std::end(m_Stage.value),
						   [&value](const stage& element) { return element.shader_stage() == value.shader_stage(); });
	if(it != std::end(m_Stage.value)) m_Stage.value.erase(it);
}
void material::erase(const blendstate& value)
{
	auto it = std::find_if(std::begin(m_BlendStates.value), std::end(m_BlendStates.value),
						   [&value](const blendstate& element) { return element.binding() == value.binding(); });
	if(it != std::end(m_BlendStates.value)) m_BlendStates.value.erase(it);
}
void material::undefine(psl::string8::view value)
{
	auto it = std::find(std::begin(m_Defines.value), std::end(m_Defines.value), value);
	if(it == std::end(m_Defines.value)) m_Defines.value.erase(it);
}

void material::from_shaders(psl::meta::library& library, std::vector<core::meta::shader*> shaderMetas)
{
	std::vector<stage>& stages = m_Stage.value;
	for(auto shader : shaderMetas)
	{
		auto it = std::find_if(std::begin(stages), std::end(stages), [&shader](const stage& stage)
							   { return stage.shader_stage() == shader->stage();});

		auto& stage = ((it != std::end(stages))? *it : stages.emplace_back());

		stage.shader(shader->stage(), shader->ID());

		std::vector<binding> bindings;
		//auto& vBindings = shader->vertex_bindings();
		//for(auto i = 0u; i < vBindings.size(); ++i)
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
		//	//	core::data::log->warn("could not resolve the UID {0}. it was neither a tag or a valid UID.", vBindings[i].buffer());

		//}
		for(const auto& descr : shader->descriptors())
		{
			binding& binding = bindings.emplace_back();
			binding.binding_slot(descr.binding());
			binding.descriptor(descr.type());
			switch(binding.descriptor())
			{
				case vk::DescriptorType::eStorageBuffer:
				case vk::DescriptorType::eStorageBufferDynamic:
				case vk::DescriptorType::eUniformBuffer:
				case vk::DescriptorType::eUniformBufferDynamic:
				if(auto uid = UID::from_string(psl::string(descr.name())); uid)
				{
					binding.buffer(uid);
				}
				else
				{
					auto res = library.find(descr.name());
					if(res)
						binding.buffer(res.value(), descr.name());
				}
				break;
				case vk::DescriptorType::eCombinedImageSampler:

				break;

				default:
				{}
			}
		}
		stage.bindings(bindings);
	}
}