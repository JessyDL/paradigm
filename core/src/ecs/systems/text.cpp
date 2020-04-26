#include "ecs/systems/text.h"
#include "psl/ecs/state.h"
#include "psl/ecs/pack.h"
#include "meta/texture.h"
#include "gfx/texture.h"
#include "gfx/context.h"
#include "gfx/buffer.h"
#include "gfx/geometry.h"
#include "gfx/bundle.h"
#include "gfx/material.h"
#include "gfx/sampler.h"
#include "gfx/pipeline_cache.h"
#include "meta/shader.h"
#include "data/material.h"
#include "data/sampler.h"

#include "ecs/components/renderable.h"
#include "ecs/components/transform.h"
#include "utility/geometry.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stdb_truetype.h"

using namespace core::ecs::systems;
namespace comp = core::ecs::components;
using namespace psl::ecs;
using namespace core::resource;
using namespace core::gfx;

text::text(psl::ecs::state& state, cache& cache, handle<context> context,
		   core::resource::handle<core::gfx::buffer> vertexBuffer,
		   core::resource::handle<core::gfx::buffer> indexBuffer,
		   core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
		   core::resource::handle<core::gfx::buffer> materialBuffer,
	core::resource::handle<core::gfx::buffer> vertexInstanceBuffer,
	core::resource::handle<core::gfx::shader_buffer_binding> materialInstanceBuffer)
	: m_Cache(cache), m_VertexBuffer(vertexBuffer), m_IndexBuffer(indexBuffer), m_Context(context)
{
	psl::static_array<stbtt_bakedchar, 96> char_data{0};

	auto view = cache.library().load("4944d446-7b9f-33b2-c6b7-468530c3afbe"_uid).value_or(psl::string_view{});

	const int width				   = 1024;
	const int height			   = 1024;
	const float character_height   = 164.0f;
	const int character_gen_offset = 32;
	psl::array<std::byte> bitmap(width * height);
	auto stbtt_res = stbtt_BakeFontBitmap((const unsigned char*)view.data(), 0, character_height,
										  reinterpret_cast<unsigned char*>(bitmap.data()), width, height,
										  character_gen_offset, static_cast<int>(char_data.size()), char_data.data());
	character_data.reserve(char_data.size());
	for(auto& c : char_data)
	{
		float xoff{};
		float yoff{};
		stbtt_aligned_quad aligned_quad{};
		stbtt_GetBakedQuad(&c, width, height, 0, &xoff, &yoff, &aligned_quad, 1);

		character_data.emplace_back(character_t{
			psl::vec4{aligned_quad.s0, aligned_quad.t0, aligned_quad.s1, aligned_quad.t1},
			psl::vec4{aligned_quad.x0, aligned_quad.y0, aligned_quad.x1, aligned_quad.y1} / character_height,
			psl::vec2{xoff, yoff} / character_height});
	}

	using meta_type = typename resource_traits<core::gfx::texture>::meta_type;
	std::unique_ptr<meta_type> metaData{std::make_unique<meta_type>()};
	metaData->width(width);
	metaData->height(height);
	metaData->depth(1);
	metaData->mip_levels(1);
	metaData->image_type(gfx::image_type::planar_2D);
	metaData->format(core::gfx::format::r8_unorm);
	metaData->usage(core::gfx::image_usage::transfer_destination | core::gfx::image_usage::sampled);
	metaData->aspect_mask(core::gfx::image_aspect::color);
	auto res = cache.library().add(psl::UID::generate(), std::move(metaData));
	psl::string8_t font_str_data;
	font_str_data.resize(bitmap.size());

	std::memcpy(font_str_data.data(), bitmap.data(), bitmap.size());
	cache.library().replace_content(res.first, font_str_data);

	m_FontTexture = cache.create_using<core::gfx::texture>(res.first, context);

	state.declare(&text::update_dynamic, this, true);
	state.declare(&text::add, this, true);
	state.declare(&text::remove, this, true);

	// create the sampler
	auto samplerData = cache.create<data::sampler>();
	samplerData->address(core::gfx::sampler_address_mode::repeat);
	samplerData->mipmaps(false);
	// samplerData->filter_max(core::gfx::filter::nearest);
	auto samplerHandle = cache.create<gfx::sampler>(m_Context, samplerData);

	auto vertShaderMeta = cache.library().get<core::meta::shader>("3982b466-58fe-4918-8735-fc6cc45378b0"_uid).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>("e1408f20-2049-4ee5-b36c-528931c71b9e"_uid).value();

	auto matData = cache.create<data::material>();
	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto stages = matData->stages();
	for(auto& stage : stages)
	{
		if(stage.shader_stage() != core::gfx::shader_stage::fragment) continue;

		auto bindings = stage.bindings();
		bindings[0].texture(m_FontTexture);
		bindings[0].sampler(samplerHandle);
		stage.bindings(bindings);
		// binding.texture()
	}
	matData->blend_states({core::data::material::blendstate(
		true, 0, core::gfx::blend_factor::source_alpha, core::gfx::blend_factor::one_minus_source_alpha,
		core::gfx::blend_op::add, core::gfx::blend_factor::one, core::gfx::blend_factor::zero, core::gfx::blend_op::add,
		core::gfx::component_bits::r | core::gfx::component_bits::g | core::gfx::component_bits::b |
			core::gfx::component_bits::a)});
	matData->stages(stages);

	auto material = m_Cache.create<core::gfx::material>(m_Context, matData, pipeline_cache, materialBuffer);

	m_Bundle = m_Cache.create<gfx::bundle>(vertexInstanceBuffer, materialInstanceBuffer);
	m_Bundle->set_material(material, 2000);
}

core::resource::handle<core::data::geometry> text::create_text(psl::string_view text)
{
	core::stream vertStream{core::stream::type::vec3};
	core::stream normStream{core::stream::type::vec3};
	core::stream colorStream{core::stream::type::vec4};
	core::stream uvStream{core::stream::type::vec2};

	auto& vertices = vertStream.as_vec3().value().get();
	auto& normals  = normStream.as_vec3().value().get();
	auto& colors   = colorStream.as_vec4().value().get();
	auto& uvs	  = uvStream.as_vec2().value().get();

	float right = 0;

	vertices.reserve(text.size() * 4);
	uvs.reserve(text.size() * 4);
	normals.resize(text.size() * 4);
	std::fill(std::begin(normals), std::end(normals), psl::vec3::one);
	colors.resize(text.size() * 4);
	std::fill(std::begin(colors), std::end(colors), psl::vec4::one);

	for(int character : text)
	{
		character -= 32; // start offset
		assert(character >= 0 && character <= character_data.size());
		const auto& data = character_data[character];

		vertices.emplace_back(psl::vec3{right + data.quad[0], data.quad[3], 0.0f});
		vertices.emplace_back(psl::vec3{right + data.quad[2], data.quad[3], 0.0f});
		vertices.emplace_back(psl::vec3{right + data.quad[2], data.quad[1], 0.0f});
		vertices.emplace_back(psl::vec3{right + data.quad[0], data.quad[1], 0.0f});

		uvs.emplace_back(psl::vec2{data.uv[0], data.uv[3]});
		uvs.emplace_back(psl::vec2{data.uv[2], data.uv[3]});
		uvs.emplace_back(psl::vec2{data.uv[2], data.uv[1]});
		uvs.emplace_back(psl::vec2{data.uv[0], data.uv[1]});

		right += data.offset.x;
	}

	auto geomData = m_Cache.create<core::data::geometry>();

	geomData->vertices(core::data::geometry::constants::POSITION, vertStream);
	geomData->vertices(core::data::geometry::constants::NORMAL, normStream);
	geomData->vertices(core::data::geometry::constants::COLOR, colorStream);
	geomData->vertices(core::data::geometry::constants::TEX, uvStream);

	std::vector<uint32_t> indexBuffer(text.size() * 6);
	for(auto i = 0, index = 0; i < text.size(); ++i)
	{
		auto offset			 = i * 4;
		indexBuffer[index++] = offset;
		indexBuffer[index++] = 1 + offset;
		indexBuffer[index++] = 2 + offset;
		indexBuffer[index++] = 2 + offset;
		indexBuffer[index++] = 3 + offset;
		indexBuffer[index++] = offset;
	}
	geomData->indices(indexBuffer);

	utility::geometry::generate_tangents(geomData);

	return geomData;
}


void text::update_dynamic(info& info,
						  pack<partial, entity, comp::text, comp::renderable, psl::ecs::filter<comp::dynamic_tag>> pack)
{
	for(auto [e, text, renderer] : pack)
	{
		renderer.geometry->recreate(create_text(text.value));
	}
}

void text::add(info& info, pack<partial, entity, comp::text, on_add<comp::text>> pack)
{

	psl::array<entity> ents;
	ents.resize(1);
	for(auto [e, text] : pack)
	{
		ents[0] = e;
		auto geomData   = create_text(text.value);
		auto geomHandle = m_Cache.create<gfx::geometry>(m_Context, geomData, m_VertexBuffer, m_IndexBuffer);
		/*info.command_buffer.create(1, comp::transform{},
			[&geomHandle, &bundle](comp::renderable& renderer) {
				renderer.geometry = geomHandle;
				renderer.bundle = bundle;
			});*/
		info.command_buffer.add_components(ents, comp::transform{},
										   [&geomHandle, &bundle = m_Bundle](comp::renderable& renderer) {
											   renderer.geometry = geomHandle;
											   renderer.bundle   = bundle;
										   });
	}
}

void text::remove(info& info, pack<partial, entity, comp::text, on_remove<comp::text>> pack)
{
	info.command_buffer.remove_components<comp::transform, comp::renderable, comp::dynamic_tag>(pack.get<entity>());
}