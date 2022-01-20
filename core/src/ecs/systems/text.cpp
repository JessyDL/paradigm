#include "ecs/systems/text.h"
#include "data/material.h"
#include "data/sampler.h"
#include "gfx/buffer.h"
#include "gfx/bundle.h"
#include "gfx/context.h"
#include "gfx/geometry.h"
#include "gfx/material.h"
#include "gfx/pipeline_cache.h"
#include "gfx/sampler.h"
#include "gfx/texture.h"
#include "meta/shader.h"
#include "meta/texture.h"
#include "psl/ecs/pack.hpp"
#include "psl/ecs/state.hpp"

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

text::text(psl::ecs::state_t& state,
		   cache_t& cache,
		   handle<context> context,
		   core::resource::handle<core::gfx::buffer_t> vertexBuffer,
		   core::resource::handle<core::gfx::buffer_t> indexBuffer,
		   core::resource::handle<core::gfx::pipeline_cache> pipeline_cache,
		   core::resource::handle<core::gfx::buffer_t> materialBuffer,
		   core::resource::handle<core::gfx::buffer_t> vertexInstanceBuffer,
		   core::resource::handle<core::gfx::shader_buffer_binding> materialInstanceBuffer) :
	m_Cache(cache),
	m_VertexBuffer(vertexBuffer), m_IndexBuffer(indexBuffer), m_Context(context)
{
	psl::static_array<stbtt_bakedchar, 96> char_data {0};

	auto view = cache.library().load("b041e1ff-09a6-dbd3-efae-aa929a7317a2"_uid).value_or(psl::string_view {});

	const int width				   = 1024;
	const int height			   = 1024;
	const float character_height   = 164.0f;
	const int character_gen_offset = 32;
	psl::array<std::byte> bitmap(width * height);
	auto stbtt_res = stbtt_BakeFontBitmap((const unsigned char*)view.data(),
										  0,
										  character_height,
										  reinterpret_cast<unsigned char*>(bitmap.data()),
										  width,
										  height,
										  character_gen_offset,
										  static_cast<int>(char_data.size()),
										  char_data.data());
	character_data.reserve(char_data.size());
	for(auto& c : char_data)
	{
		float xoff {};
		float yoff {};
		stbtt_aligned_quad aligned_quad {};
		stbtt_GetBakedQuad(&c, width, height, 0, &xoff, &yoff, &aligned_quad, 1);

		character_data.emplace_back(character_t {
		  psl::vec4 {aligned_quad.s0, aligned_quad.t0, aligned_quad.s1, aligned_quad.t1},
		  psl::vec4 {aligned_quad.x0, aligned_quad.y0, aligned_quad.x1, aligned_quad.y1} / character_height,
		  psl::vec2 {xoff, yoff} / character_height});
	}

	using meta_type = typename resource_traits<core::gfx::texture_t>::meta_type;
	std::unique_ptr<meta_type> metaData {std::make_unique<meta_type>()};
	metaData->width(width);
	metaData->height(height);
	metaData->depth(1);
	metaData->mip_levels(1);
	metaData->image_type(gfx::image_type::planar_2D);
	metaData->format(core::gfx::format_t::r8_unorm);
	metaData->usage(core::gfx::image_usage::transfer_destination | core::gfx::image_usage::sampled);
	metaData->aspect_mask(core::gfx::image_aspect::color);
	auto res = cache.library().add(psl::UID::generate(), std::move(metaData));
	psl::string8_t font_str_data;
	font_str_data.resize(bitmap.size());

	std::memcpy(font_str_data.data(), bitmap.data(), bitmap.size());
	cache.library().replace_content(res.first, font_str_data);

	m_FontTexture = cache.create_using<core::gfx::texture_t>(res.first, context);

	state.declare<"text::update_dynamic">(&text::update_dynamic, this, true);
	state.declare<"text::add">(&text::add, this, true);
	state.declare<"text::remove">(&text::remove, this, true);

	// create the sampler
	auto samplerData = cache.create<data::sampler_t>();
	samplerData->address(core::gfx::sampler_address_mode::repeat);
	samplerData->mipmaps(false);
	// samplerData->filter_max(core::gfx::filter::nearest);
	auto samplerHandle = cache.create<gfx::sampler_t>(m_Context, samplerData);

	auto vertShaderMeta = cache.library().get<core::meta::shader>("0f48f21f-f707-06b5-5c66-83ff0d53c5a1"_uid).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>("db43dbb4-04ce-65f5-7415-d1fbc90d1aad"_uid).value();

	auto matData = cache.create<data::material_t>();
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
	matData->blend_states(
	  {core::data::material_t::blendstate(true,
										0,
										core::gfx::blend_factor::source_alpha,
										core::gfx::blend_factor::one_minus_source_alpha,
										core::gfx::blend_op::add,
										core::gfx::blend_factor::one,
										core::gfx::blend_factor::zero,
										core::gfx::blend_op::add,
										core::gfx::component_bits::r | core::gfx::component_bits::g |
										  core::gfx::component_bits::b | core::gfx::component_bits::a)});
	matData->stages(stages);
	matData->cull_mode(core::gfx::cullmode::none);

	auto material = m_Cache.create<core::gfx::material_t>(m_Context, matData, pipeline_cache, materialBuffer);

	m_Bundle = m_Cache.create<gfx::bundle>(vertexInstanceBuffer, materialInstanceBuffer);
	m_Bundle->set_material(material, 2000);
}

core::resource::handle<core::data::geometry_t> text::create_text(psl::string_view text)
{
	core::stream vertStream {core::stream::type::vec3};
	core::stream normStream {core::stream::type::vec3};
	core::stream colorStream {core::stream::type::vec4};
	core::stream uvStream {core::stream::type::vec2};

	auto& vertices = vertStream.as_vec3().value().get();
	auto& normals  = normStream.as_vec3().value().get();
	auto& colors   = colorStream.as_vec4().value().get();
	auto& uvs	   = uvStream.as_vec2().value().get();

	float right = 0;
	float up	= 1.0f;

	// validate for illegal characters in input.
	{
		const auto max_char = character_data.size() + 32;
		for(int character : text)
		{
			assert((character >= 32 && character < max_char) || character == '\n' || character == '\t');
		}
	}

	auto size = std::accumulate(std::begin(text), std::end(text), size_t {0u}, [](size_t sum, int character) -> size_t {
		return sum + ((character == '\n') ? 0u : (character == '\t') ? 4u : 1u);
	});

	vertices.reserve(size * 4);
	uvs.reserve(size * 4);

	auto insert_character = [&vertices, &uvs](float& right, float up, const auto& data) {
		vertices.emplace_back(psl::vec3 {right + data.quad[0], up - data.quad[3], 0.0f});
		vertices.emplace_back(psl::vec3 {right + data.quad[2], up - data.quad[3], 0.0f});
		vertices.emplace_back(psl::vec3 {right + data.quad[2], up - data.quad[1], 0.0f});
		vertices.emplace_back(psl::vec3 {right + data.quad[0], up - data.quad[1], 0.0f});

		uvs.emplace_back(psl::vec2 {data.uv[0], data.uv[3]});
		uvs.emplace_back(psl::vec2 {data.uv[2], data.uv[3]});
		uvs.emplace_back(psl::vec2 {data.uv[2], data.uv[1]});
		uvs.emplace_back(psl::vec2 {data.uv[0], data.uv[1]});

		right += data.offset.x;
	};

	size_t text_count {0u};
	for(int character : text)
	{
		if(character == '\n')
		{
			right = 0.0f;
			up -= 1.0f;
			text_count = 0;
			continue;
		}

		if(character == '\t')
		{
			character		 = ' ' - 32;
			const auto& data = character_data[character];

			for(auto i = 4 - text_count % 4; i > 0; --i)
			{
				insert_character(right, up, data);
				++text_count;
			}
		}
		else
		{
			const auto& data = character_data[character - 32];
			insert_character(right, up, data);
			++text_count;
		}
	}

	const auto character_count = vertices.size() / 4;

	normals.resize(vertices.size());
	std::fill(std::begin(normals), std::end(normals), psl::vec3::one);
	colors.resize(vertices.size());
	std::fill(std::begin(colors), std::end(colors), psl::vec4::one);

	auto geomData = m_Cache.create<core::data::geometry_t>();

	geomData->vertices(core::data::geometry_t::constants::POSITION, vertStream);
	geomData->vertices(core::data::geometry_t::constants::NORMAL, normStream);
	geomData->vertices(core::data::geometry_t::constants::COLOR, colorStream);
	geomData->vertices(core::data::geometry_t::constants::TEX, uvStream);

	std::vector<uint32_t> indexBuffer(character_count * 6);
	for(auto i = 0u, index = 0u; i < character_count; ++i)
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


void text::update_dynamic(info_t& info,
						  pack<partial, entity, comp::text, comp::renderable, psl::ecs::filter<comp::dynamic_tag>> pack)
{
	for(auto [e, text, renderer] : pack)
	{
		renderer.geometry->recreate(create_text(*text.value));
	}
}

void text::add(info_t& info, pack<partial, entity, comp::text, on_add<comp::text>> pack)
{
	psl::array<entity> ents;
	ents.resize(1);
	for(auto [e, text] : pack)
	{
		ents[0]			= e;
		auto geomData	= create_text(*text.value);
		auto geomHandle = m_Cache.create<gfx::geometry_t>(m_Context, geomData, m_VertexBuffer, m_IndexBuffer);
		/*info.command_buffer.create(1, comp::transform{},
			[&geomHandle, &bundle](comp::renderable& renderer) {
				renderer.geometry = geomHandle;
				renderer.bundle = bundle;
			});*/
		info.command_buffer.add_components(ents, [&geomHandle, &bundle = m_Bundle](comp::renderable& renderer) {
			renderer.geometry = geomHandle;
			renderer.bundle	  = bundle;
		});
	}
}

void text::remove(info_t& info, pack<partial, entity, comp::text, on_remove<comp::text>> pack)
{
	info.command_buffer.remove_components<comp::renderable>(pack.get<entity>());
}