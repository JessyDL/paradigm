#include <SPIRV/GlslangToSpv.h>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <spirv_glsl.hpp>
#include <spirv_reflect.hpp>

#include "details/spirv.hpp"
#include "stdafx.h"

#include "psl/array.hpp"
#include "psl/array_view.hpp"
#include "psl/string_utils.hpp"
#include "psl/ustring.hpp"
tools::_internal::glslang_manager_t tools::_internal::glslang_manager {};

template <typename TargetType>
constexpr auto narrow_cast(auto v) -> TargetType {
	if(static_cast<TargetType>(v) != v) {
		throw std::runtime_error(fmt::format("narrow_cast<{}> failed, value {} is out of range for type {}",
											 typeid(TargetType).name(),
											 v,
											 typeid(TargetType).name()));
	}
	return static_cast<TargetType>(v);
}

namespace tools {
namespace _internal {
	glslang_manager_t::glslang_manager_t() {
		glslang_initialize_process();
	}
	glslang_manager_t::~glslang_manager_t() {
		glslang_finalize_process();
	}
}	 // namespace _internal

constexpr glslang_stage_t get_stage(shader_stage_t stage) {
	switch(stage) {
	case shader_stage_t::comp:
		return glslang_stage_t::GLSLANG_STAGE_COMPUTE;
	case shader_stage_t::vert:
		return glslang_stage_t::GLSLANG_STAGE_VERTEX;
	case shader_stage_t::tesc:
		return glslang_stage_t::GLSLANG_STAGE_TESSCONTROL;
	case shader_stage_t::tese:
		return glslang_stage_t::GLSLANG_STAGE_TESSEVALUATION;
	case shader_stage_t::geom:
		return glslang_stage_t::GLSLANG_STAGE_GEOMETRY;
	case shader_stage_t::frag:
		return glslang_stage_t::GLSLANG_STAGE_FRAGMENT;
	default:
		return glslang_stage_t::GLSLANG_STAGE_COUNT;
	}
}

bool reflect_spirv(glsl_compile_result_t& result) {
	auto spirv_data = (uint32_t const*)result.spirv.data();
	auto spirv_size = result.spirv.size() / sizeof(uint32_t);
	spirv_cross::CompilerReflection module(spirv_data, spirv_size);
	auto const& resources = module.get_shader_resources();

	auto parse_attributes = [&module,
							 &result](auto const& source) mutable -> psl::array<core::meta::shader::attribute> {
		if(!result) {
			return {};
		}
		psl::array<core::meta::shader::attribute> attributes {};
		for(spirv_cross::Resource const& value : source) {
			auto& attribute = attributes.emplace_back();
			attribute.name(value.name);
			auto const& type = module.get_type(value.type_id);
			attribute.count(type.array.size() > 0 ? type.array[0] : type.columns);
			attribute.stride(type.columns * type.width / 8 * type.vecsize / attribute.count());
			attribute.location(module.get_decoration(value.id, spv::DecorationLocation));

			using base_type = spirv_cross::SPIRType::BaseType;
			switch(type.basetype) {
			case base_type::Float:
				attribute.format(
				  core::gfx::format_t(std::to_underlying(core::gfx::format_t::r32_sfloat) + 3 * (type.vecsize - 1)));
				break;
			case base_type::Boolean:
			case base_type::Double:
				attribute.format(core::gfx::format_t::undefined);
				break;
			case base_type::Int:
				attribute.format(core::gfx::format_t::r32_sint);
				break;
			case base_type::UInt:
				attribute.format(core::gfx::format_t::r32_uint);
				break;
			default:
				result.success = false;
				result.shader  = {};
				result.messages.emplace_back(fmt::format("reflection error, could not decode the attributes type {}",
														 std::to_underlying(type.basetype)),
											 true);
				return {};
			}
		}

		std::sort(std::begin(attributes), std::end(attributes), [](auto const& l, auto const& r) {
			return l.location() < r.location();
		});
		return attributes;
	};
	result.shader.inputs  = parse_attributes(resources.stage_inputs);
	result.shader.outputs = parse_attributes(resources.stage_outputs);

	auto parse_descriptors = [&module,
							  &result](auto const& source) mutable -> psl::array<core::meta::shader::descriptor> {
		if(!result) {
			return {};
		}
		constexpr auto qualifier = [](auto const& module,
									  auto id) noexcept -> core::meta::shader::descriptor::dependency {
			if(module.get_decoration(id, spv::DecorationNonReadable)) {
				return (module.get_decoration(id, spv::DecorationNonWritable))
						 ? core::meta::shader::descriptor::dependency::inout
						 : core::meta::shader::descriptor::dependency::out;
			} else {
				return (module.get_decoration(id, spv::DecorationNonWritable))
						 ? core::meta::shader::descriptor::dependency::out
						 : core::meta::shader::descriptor::dependency::inout;
			}
		};
		psl::array<core::meta::shader::descriptor> descriptors {};
		for(spirv_cross::Resource const& value : source) {
			auto& descriptor = descriptors.emplace_back();
			descriptor.name(value.name);
			descriptor.set(module.get_decoration(value.id, spv::DecorationDescriptorSet));
			descriptor.binding(module.get_decoration(value.id, spv::DecorationBinding));
			descriptor.qualifier(qualifier(module, value.id));

			auto const& type = module.get_type(value.type_id);
			using base_type	 = spirv_cross::SPIRType::BaseType;

			switch(type.basetype) {
			case base_type::Struct: {
				auto storage =
				  (type.storage == spv::StorageClassUniform && module.get_decoration(type.self, spv::DecorationBlock))
					? spv::StorageClassUniform
					: spv::StorageClassStorageBuffer;

				switch(storage) {
				case spv::StorageClassUniform:
					descriptor.type(psl::utility::string::contains(value.name, "_DYNAMIC_")
									  ? core::gfx::binding_type::uniform_buffer_dynamic
									  : core::gfx::binding_type::uniform_buffer);
					break;
				case spv::StorageClassStorageBuffer:
					descriptor.type(psl::utility::string::contains(value.name, "_DYNAMIC_")
									  ? core::gfx::binding_type::storage_buffer_dynamic
									  : core::gfx::binding_type::storage_buffer);
					break;
				default:
					throw std::runtime_error("unhandled type");
				}
			} break;
			case base_type::SampledImage:
				assert(type.storage == spv::StorageClassUniformConstant);
				descriptor.type(core::gfx::binding_type::combined_image_sampler);
				break;
			case base_type::Sampler:
				assert(type.storage == spv::StorageClassUniformConstant);
				descriptor.type(core::gfx::binding_type::sampler);
				break;
			case base_type::Image:
				// image
				if(type.storage == spv::StorageClassUniformConstant && type.image.sampled == 2) {
					descriptor.type(core::gfx::binding_type::storage_image);
				}
				// separate image
				else if(type.storage == spv::StorageClassUniformConstant && type.image.sampled == 1) {
					descriptor.type(core::gfx::binding_type::sampled_image);
				} else {
					result.shader  = {};
					result.success = false;
					result.messages.emplace_back(fmt::format("reflection error, unknown storage {} for image type",
															 std::to_underlying(type.storage)),
												 true);
					return {};
				}
				break;
			default:
				result.shader  = {};
				result.success = false;
				result.messages.emplace_back(
				  fmt::format("reflection error, unknown descriptor type {}", std::to_underlying(type.basetype)), true);
				return {};
			}

			auto decode_member = [&](spirv_cross::CompilerReflection const& module,
									 spirv_cross::SPIRType const& type,
									 uint32_t i,
									 auto& recursiveFn) -> core::meta::shader::member {
				auto const& member_type_id = type.member_types[i];
				auto const& member_type	   = module.get_type(member_type_id);
				auto member_name		   = module.get_member_name(type.self, i);
				uint32_t offset			   = module.get_member_decoration(type.self, i, spv::DecorationOffset);

				uint32_t stride = 0;
				uint32_t count	= 1;
				if(member_type.array.size() > 0) {
					stride = module.type_struct_member_array_stride(type, i);
					count  = member_type.array[0];
				} else if(member_type.columns > 1) {
					stride = module.type_struct_member_matrix_stride(type, i);
					count  = member_type.columns;
				} else {
					// todo(jdl): we should support up-to 64-bit here, but for now we only support 32-bit
					// which should be "good enough" for now. Luckily we will get a clear exception if we
					// need a larger size.
					stride = narrow_cast<uint32_t>(module.get_declared_struct_member_size(type, i));
				}

				core::meta::shader::member member {};
				member.name(member_name);
				member.stride(stride);
				member.count(count);
				member.offset(offset);

				// if the member is a struct, we need to recursively decode it
				if(member_type.basetype == spirv_cross::SPIRType::BaseType::Struct) {
					psl::array<core::meta::shader::member> members {};
					for(auto i = 0; i < member_type.member_types.size(); ++i) {
						members.emplace_back(recursiveFn(module, member_type, uint32_t(i), recursiveFn));
					}
					member.members(std::move(members));
				}
				return member;
			};

			// next up parse all members
			psl::array<core::meta::shader::member> members {};
			for(auto i = 0; i < type.member_types.size(); ++i) {
				members.emplace_back(decode_member(module, type, uint32_t(i), decode_member));
			}
			descriptor.members(std::move(members));
		}
		return descriptors;
	};

	// some compilers are missing an implementation for .append_range
	auto append_range = [](auto& target, auto const& source) {
		target.insert(std::end(target), std::begin(source), std::end(source));
	};

	auto descriptors = parse_descriptors(resources.uniform_buffers);
	append_range(descriptors, parse_descriptors(resources.storage_buffers));
	append_range(descriptors, parse_descriptors(resources.sampled_images));
	append_range(descriptors, parse_descriptors(resources.separate_images));
	append_range(descriptors, parse_descriptors(resources.separate_samplers));
	append_range(descriptors, parse_descriptors(resources.storage_images));
	result.shader.descriptors = std::move(descriptors);

	auto const& entries = module.get_entry_points_and_stages();
	if(entries.size() != 1) {
		result.shader  = glsl_compile_result_t::shader_t();
		result.success = false;
		return result;
	}

	switch(entries[0].execution_model) {
	case spv::ExecutionModelVertex:
		result.shader.stage = core::gfx::shader_stage::vertex;
		break;
	case spv::ExecutionModelFragment:
		result.shader.stage = core::gfx::shader_stage::fragment;
		break;
	case spv::ExecutionModelGeometry:
		result.shader.stage = core::gfx::shader_stage::geometry;
		break;
	case spv::ExecutionModelTessellationControl:
		result.shader.stage = core::gfx::shader_stage::tesselation_control;
		break;
	case spv::ExecutionModelTessellationEvaluation:
		result.shader.stage = core::gfx::shader_stage::tesselation_evaluation;
		break;
	case spv::ExecutionModelGLCompute:
		result.shader.stage = core::gfx::shader_stage::compute;
		break;
	default:
		result.shader  = glsl_compile_result_t::shader_t();
		result.success = false;
		return result;
	}
	return result;
}

bool compileShaderToSPIRV_Vulkan(glslang_stage_t stage,
								 char const* shaderSource,
								 bool optimize,
								 tools::glsl_compile_result_t& result) {
	glslang_input_t const input = {
	  .language							 = GLSLANG_SOURCE_GLSL,
	  .stage							 = stage,
	  .client							 = GLSLANG_CLIENT_VULKAN,
	  .client_version					 = GLSLANG_TARGET_VULKAN_1_3,
	  .target_language					 = GLSLANG_TARGET_SPV,
	  .target_language_version			 = GLSLANG_TARGET_SPV_1_3,
	  .code								 = shaderSource,
	  .default_version					 = 100,
	  .default_profile					 = GLSLANG_NO_PROFILE,
	  .force_default_version_and_profile = false,
	  .forward_compatible				 = true,
	  .messages							 = GLSLANG_MSG_DEFAULT_BIT,
	  .resource							 = glslang_default_resource(),
	};

	glslang_shader_t* shader = glslang_shader_create(&input);

	if(!glslang_shader_preprocess(shader, &input)) {
		result.messages.emplace_back(fmt::format("preprocessing failure: {}\n{}",
												 glslang_shader_get_info_log(shader),
												 glslang_shader_get_info_debug_log(shader)),
									 true);
		glslang_shader_delete(shader);
		return false;
	}

	if(!glslang_shader_parse(shader, &input)) {
		result.messages.emplace_back(fmt::format("compilation failure: {}\n{}\n{}",
												 glslang_shader_get_info_log(shader),
												 glslang_shader_get_info_debug_log(shader),
												 glslang_shader_get_preprocessed_code(shader)),
									 true);
		glslang_shader_delete(shader);
		return false;
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if(!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		result.messages.emplace_back(fmt::format("linking failure: {}\n{}",
												 glslang_program_get_info_log(program),
												 glslang_program_get_info_debug_log(program)),
									 true);
		glslang_program_delete(program);
		glslang_shader_delete(shader);
		return false;
	}
	glslang_spv_options_t spv_options {};
	spv_options.disable_optimizer	= !optimize;
	spv_options.validate			= true;
	spv_options.generate_debug_info = !optimize;
	glslang_program_SPIRV_generate_with_options(program, stage, &spv_options);
	result.spirv.resize(glslang_program_SPIRV_get_size(program) * sizeof(uint32_t));
	glslang_program_SPIRV_get(program, (uint32_t*)(result.spirv.data()));


	char const* spirv_messages = glslang_program_SPIRV_get_messages(program);
	if(spirv_messages) {
		result.messages.emplace_back(spirv_messages, false);
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return true;
}
glsl_compile_result_t
glsl_compile(psl::string_view source, shader_stage_t type, bool optimize, std::optional<size_t> gles_version) {
	auto const stage = get_stage(type);
	glsl_compile_result_t result {true};
	if(stage == glslang_stage_t::GLSLANG_STAGE_COUNT) {
		result.messages.emplace_back(fmt::format("unknown shader_stage_t value '{}'", std::to_underlying(type)), true);
		result.success = false;
		return result;
	}
	if(!compileShaderToSPIRV_Vulkan(stage, source.data(), optimize, result)) {
		result.success = false;
		return result;
	}

	if(gles_version) {
		spirv_cross::CompilerGLSL gles_compiler((uint32_t const*)result.spirv.data(),
												result.spirv.size() / sizeof(uint32_t));

		spirv_cross::CompilerGLSL::Options options;
		options.version = narrow_cast<uint32_t>(gles_version.value());
		options.es		= true;
		gles_compiler.set_common_options(options);

		result.gles = gles_compiler.compile();
	}

	try {
		if(!reflect_spirv(result)) {
			result.success = false;
			result.shader  = {};
			result.messages.emplace_back("Reflection failure", true);
			return result;
		}
	} catch(spirv_cross::CompilerError e) {
		result.success = false;
		result.shader  = {};
		result.messages.emplace_back(e.what(), true);
	}
	return result;
}
}	 // namespace tools
