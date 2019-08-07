
#include "meta/shader.h"

using namespace core::meta;
using namespace psl::serialization;
const uint64_t shader::polymorphic_identity{register_polymorphic<shader>()};

// vertex attribute
shader::vertex::attribute::attribute() {}
shader::vertex::attribute::~attribute() {}

uint32_t shader::vertex::attribute::location() const noexcept { return m_Location.value; }
vk::Format shader::vertex::attribute::format() const noexcept { return m_Format.value; }
uint32_t shader::vertex::attribute::offset() const noexcept { return m_Offset.value; }

void shader::vertex::attribute::location(uint32_t value) { m_Location.value = value; }
void shader::vertex::attribute::format(vk::Format value) { m_Format.value = value; }
void shader::vertex::attribute::offset(uint32_t value) { m_Offset.value = value; }
// vertex binding
shader::vertex::binding::binding() {}
shader::vertex::binding::~binding() {}

uint32_t shader::vertex::binding::binding_slot() const noexcept { return m_Binding.value; }
uint32_t shader::vertex::binding::size() const noexcept { return m_Size.value; }
core::gfx::vertex_input_rate shader::vertex::binding::input_rate() const noexcept { return m_InputRate.value; }
psl::string8_t shader::vertex::binding::buffer() const noexcept { return m_Buffer.value; }
const std::vector<shader::vertex::attribute>& shader::vertex::binding::attributes() const noexcept
{
	return m_Attributes.value;
}

void shader::vertex::binding::binding_slot(uint32_t value) { m_Binding.value = value; }
void shader::vertex::binding::size(uint32_t value) { m_Size.value = value; }
void shader::vertex::binding::input_rate(core::gfx::vertex_input_rate value) { m_InputRate.value = value; }
void shader::vertex::binding::buffer(psl::string8_t value) { m_Buffer.value = value; }
void shader::vertex::binding::attributes(const std::vector<shader::vertex::attribute>& value)
{
	m_Attributes.value = value;
}

void shader::vertex::binding::set(attribute value)
{
	auto it = std::find_if(std::begin(m_Attributes.value), std::end(m_Attributes.value), [&value](const attribute& attribute) {
							   return attribute.location() == value.location() &&
									  attribute.format() == value.format() && attribute.offset() == value.offset();
		});
	if(it == std::end(m_Attributes.value))
	{
		m_Attributes.value.emplace_back(std::move(value));
	}
	else
	{
		*it = value;
	}
}
bool shader::vertex::binding::erase(attribute value)
{
	auto it = std::find_if(std::begin(m_Attributes.value), std::end(m_Attributes.value),
						   [&value](const attribute& attribute) {
							   return attribute.location() == value.location() &&
									  attribute.format() == value.format() && attribute.offset() == value.offset();
						   });
	if(it == std::end(m_Attributes.value)) return false;

	m_Attributes.value.erase(it);
	return true;
}

// instance
shader::instance::instance() {}
shader::instance::~instance() {}

const std::vector<shader::instance::element>& shader::instance::elements() const noexcept { return m_Elements.value; }
std::vector<uint8_t> shader::instance::default_value() const noexcept
{ 
	std::vector<uint8_t> res;
	for(const auto& element : m_Elements.value)
	{
		res.insert(std::end(res), std::begin(element.default_value()), std::end(element.default_value()));
	}
	return res;
}
uint32_t shader::instance::size() const noexcept { return m_Size.value; }

void shader::instance::elements(const std::vector<shader::instance::element>& value) { m_Elements.value = value; }
void shader::instance::size(uint32_t value) { m_Size.value = value; }


void shader::instance::set(element value)
{
	auto it = std::find_if(std::begin(m_Elements.value), std::end(m_Elements.value),
						   [&value](const instance::element& element) { return element.name() == value.name(); });
	if(it == std::end(m_Elements.value))
	{
		m_Elements.value.emplace_back(std::move(value));
	}
	else
	{
		*it = value;
	}
}
bool shader::instance::erase(element value)
{
	auto it = std::find_if(std::begin(m_Elements.value), std::end(m_Elements.value),
						   [&value](const instance::element& element) { return element.name() == value.name(); });
	if(it == std::end(m_Elements.value)) return false;

	m_Elements.value.erase(it);
	return true;

}
bool shader::instance::erase(psl::string8::view element_name)
{
	auto it =
		std::find_if(std::begin(m_Elements.value), std::end(m_Elements.value),
					 [&element_name](const instance::element& element) { return element.name() == element_name; });
	if(it == std::end(m_Elements.value)) return false;

	m_Elements.value.erase(it);
	return true;
}

// instance element
shader::instance::element::element() {}
shader::instance::element::~element() {}

psl::string8::view shader::instance::element::name() const noexcept { return m_Name.value; }
vk::Format shader::instance::element::format() const noexcept { return m_Format.value; }
uint32_t shader::instance::element::offset() const noexcept { return m_Offset.value; }
const std::vector<uint8_t>& shader::instance::element::default_value() const noexcept { return m_Default.value; }

void shader::instance::element::name(psl::string8::view value) { m_Name.value = value; }
void shader::instance::element::format(vk::Format value) { m_Format.value = value; }
void shader::instance::element::offset(uint32_t value) { m_Offset.value = value; }
void shader::instance::element::default_value(const std::vector<uint8_t>& value) { m_Default.value = value; }

// descriptor
shader::descriptor::descriptor() {}
shader::descriptor::~descriptor() {}

bool shader::descriptor::has_default_value() const noexcept
{
	return std::any_of(std::begin(m_SubElements.value), std::end(m_SubElements.value), [](const auto& element) { return element.default_value().size() > 0;});
}

uint32_t shader::descriptor::binding() const noexcept { return m_Binding.value; }
uint32_t shader::descriptor::size() const noexcept { return m_Size.value; }
psl::string8::view shader::descriptor::name() const noexcept { return m_Name.value; }
std::vector<uint8_t> shader::descriptor::default_value() const noexcept
{
	std::vector<uint8_t> res;
	for(const auto& element : m_SubElements.value)
	{
		res.insert(std::end(res), std::begin(element.default_value()), std::end(element.default_value()));
	}
	return res;
}
vk::DescriptorType shader::descriptor::type() const noexcept { return m_Type.value; }
const std::vector<shader::instance::element>& shader::descriptor::sub_elements() const noexcept
{
	return m_SubElements.value;
}

void shader::descriptor::binding(uint32_t value) { m_Binding.value = value; }
void shader::descriptor::size(uint32_t value) { m_Size.value = value; }
void shader::descriptor::name(psl::string8::view value) { m_Name.value = value; }
void shader::descriptor::type(vk::DescriptorType value) { m_Type.value = value; }
void shader::descriptor::sub_elements(const std::vector<shader::instance::element>& value)
{
	m_SubElements.value = value;
}


void shader::descriptor::set(instance::element value)
{
	auto it = std::find_if(std::begin(m_SubElements.value), std::end(m_SubElements.value),
						   [&value](const instance::element& element) { return element.name() == value.name(); });
	if(it == std::end(m_SubElements.value))
	{
		m_SubElements.value.emplace_back(std::move(value));
	}
	else
	{
		*it = value;
	}
}
bool shader::descriptor::erase(instance::element value)
{
	auto it = std::find_if(std::begin(m_SubElements.value), std::end(m_SubElements.value),
						   [&value](const instance::element& element) { return element.name() == value.name(); });
	if(it == std::end(m_SubElements.value)) return false;

	m_SubElements.value.erase(it);
	return true;
}

// shader

core::gfx::shader_stage shader::stage() const noexcept { return m_Stage.value; }
const std::vector<shader::vertex::binding>& shader::vertex_bindings() const noexcept { return m_VertexBindings.value; }
const std::vector<shader::descriptor>& shader::descriptors() const noexcept { return m_Descriptors.value; }

void shader::stage(core::gfx::shader_stage value) noexcept { m_Stage.value = value; }
void shader::vertex_bindings(const std::vector<shader::vertex::binding>& value) { m_VertexBindings.value = value; }
void shader::descriptors(const std::vector<descriptor>& value) { m_Descriptors.value = value; }

void shader::set(shader::descriptor value)
{
	auto it =
		std::find_if(std::begin(m_Descriptors.value), std::end(m_Descriptors.value),
		[&value](const shader::descriptor& descriptor) { return descriptor.binding() == value.binding(); });
	if(it == std::end(m_Descriptors.value))
	{
		m_Descriptors.value.emplace_back(std::move(value));
	}
	else
	{
		*it = value;
	}
}

void shader::set(shader::vertex::binding value)
{
	auto it = std::find_if(std::begin(m_VertexBindings.value), std::end(m_VertexBindings.value),
		[&value](const shader::vertex::binding& binding) { return binding.binding_slot() == value.binding_slot(); });
	if(it == std::end(m_VertexBindings.value))
	{
		m_VertexBindings.value.emplace_back(std::move(value));
	}
	else
	{
		*it = value;
	}
}

bool shader::erase(shader::vertex::binding value)
{
	auto it = std::find_if(std::begin(m_VertexBindings.value), std::end(m_VertexBindings.value),
		[&value](const shader::vertex::binding& binding) { return binding.binding_slot() == value.binding_slot(); });
	if(it == std::end(m_VertexBindings.value)) return false;

	m_VertexBindings.value.erase(it);
	return true;
}

bool shader::erase(shader::descriptor value)
{
	auto it =
		std::find_if(std::begin(m_Descriptors.value), std::end(m_Descriptors.value),
		[&value](const shader::descriptor& descriptor) { return descriptor.binding() == value.binding(); });
	if(it == std::end(m_Descriptors.value)) return false;

	m_Descriptors.value.erase(it);
	return true;
}


std::vector<shader::vertex::binding> shader::instance_bindings() const noexcept
{
	std::vector<shader::vertex::binding> res;
	for(const auto& binding : m_VertexBindings.value)
	{
		if(psl::string8::view(binding.buffer()).substr(0, 9) != "INSTANCE_")
			continue;
		res.emplace_back(binding);
	}
	return res;
}
std::optional<shader::descriptor> shader::material_data() const noexcept
{
	auto it = std::find_if(std::begin(m_Descriptors.value), std::end(m_Descriptors.value), [](const auto& descriptor)
	{
		return(descriptor.name() == "MATERIAL_DATA");
	});
	std::optional<shader::descriptor> res{std::nullopt};
	if(it != std::end(m_Descriptors.value))
		res = *it;
	return res;
}