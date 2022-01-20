#if defined(GL_ENABLE_ALL_DEFINES) && !defined(_INCL_GL_ENABLE_ALL_DEFINES)
#define _INCL_GL_ENABLE_ALL_DEFINES
#if !defined(GL_AMD_compressed_3DC_texture)
#define GL_3DC_X_AMD 0x87F9
#define GL_3DC_XY_AMD 0x87FA
#endif // GL_AMD_compressed_3DC_texture

#if !defined(GL_AMD_compressed_ATC_texture)
#define GL_ATC_RGB_AMD 0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD 0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD 0x87EE
#endif // GL_AMD_compressed_ATC_texture

#if !defined(GL_AMD_framebuffer_multisample_advanced)
#define GL_RENDERBUFFER_STORAGE_SAMPLES_AMD 0x91B2
#define GL_MAX_COLOR_FRAMEBUFFER_SAMPLES_AMD 0x91B3
#define GL_MAX_COLOR_FRAMEBUFFER_STORAGE_SAMPLES_AMD 0x91B4
#define GL_MAX_DEPTH_STENCIL_FRAMEBUFFER_SAMPLES_AMD 0x91B5
#define GL_NUM_SUPPORTED_MULTISAMPLE_MODES_AMD 0x91B6
#define GL_SUPPORTED_MULTISAMPLE_MODES_AMD 0x91B7
#endif // GL_AMD_framebuffer_multisample_advanced

#if !defined(GL_AMD_performance_monitor)
#define GL_COUNTER_TYPE_AMD 0x8BC0
#define GL_COUNTER_RANGE_AMD 0x8BC1
#define GL_UNSIGNED_INT64_AMD 0x8BC2
#define GL_PERCENTAGE_AMD 0x8BC3
#define GL_PERFMON_RESULT_AVAILABLE_AMD 0x8BC4
#define GL_PERFMON_RESULT_SIZE_AMD 0x8BC5
#define GL_PERFMON_RESULT_AMD 0x8BC6
#endif // GL_AMD_performance_monitor

#if !defined(GL_AMD_program_binary_Z400)
#define GL_Z400_BINARY_AMD 0x8740
#endif // GL_AMD_program_binary_Z400

#if !defined(GL_ANGLE_depth_texture) && !defined(GL_OES_depth_texture)
#define GL_DEPTH_COMPONENT 0x1902
#define GL_UNSIGNED_SHORT 0x1403
#endif // GL_ANGLE_depth_texture && GL_OES_depth_texture

#if !defined(GL_ANGLE_depth_texture) && !defined(GL_OES_packed_depth_stencil)
#define GL_DEPTH_STENCIL_OES 0x84F9
#define GL_UNSIGNED_INT_24_8_OES 0x84FA
#endif // GL_ANGLE_depth_texture && GL_OES_packed_depth_stencil

#if !defined(GL_ANGLE_depth_texture) && !defined(GL_OES_depth_texture) && !defined(GL_OES_element_index_uint)
#define GL_UNSIGNED_INT 0x1405
#endif // GL_ANGLE_depth_texture && GL_OES_depth_texture && GL_OES_element_index_uint

#if !defined(GL_ANGLE_depth_texture)
#define GL_DEPTH_COMPONENT16 0x81A5
#endif // GL_ANGLE_depth_texture

#if !defined(GL_ANGLE_depth_texture) && !defined(GL_OES_depth32) && !defined(GL_OES_required_internalformat)
#define GL_DEPTH_COMPONENT32_OES 0x81A7
#endif // GL_ANGLE_depth_texture && GL_OES_depth32 && GL_OES_required_internalformat

#if !defined(GL_ANGLE_depth_texture) && !defined(GL_OES_packed_depth_stencil) &&                                       \
	!defined(GL_OES_required_internalformat)
#define GL_DEPTH24_STENCIL8_OES 0x88F0
#endif // GL_ANGLE_depth_texture && GL_OES_packed_depth_stencil && GL_OES_required_internalformat

#if !defined(GL_ANGLE_framebuffer_blit)
#define GL_READ_FRAMEBUFFER_ANGLE 0x8CA8
#define GL_DRAW_FRAMEBUFFER_ANGLE 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_ANGLE 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_ANGLE 0x8CAA
#endif // GL_ANGLE_framebuffer_blit

#if !defined(GL_ANGLE_framebuffer_multisample)
#define GL_RENDERBUFFER_SAMPLES_ANGLE 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE 0x8D56
#define GL_MAX_SAMPLES_ANGLE 0x8D57
#endif // GL_ANGLE_framebuffer_multisample

#if !defined(GL_ANGLE_instanced_arrays)
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ANGLE 0x88FE
#endif // GL_ANGLE_instanced_arrays

#if !defined(GL_ANGLE_pack_reverse_row_order)
#define GL_PACK_REVERSE_ROW_ORDER_ANGLE 0x93A4
#endif // GL_ANGLE_pack_reverse_row_order

#if !defined(GL_ANGLE_program_binary)
#define GL_PROGRAM_BINARY_ANGLE 0x93A6
#endif // GL_ANGLE_program_binary

#if !defined(GL_ANGLE_texture_compression_dxt3)
#define GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE 0x83F2
#endif // GL_ANGLE_texture_compression_dxt3

#if !defined(GL_ANGLE_texture_compression_dxt5)
#define GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE 0x83F3
#endif // GL_ANGLE_texture_compression_dxt5

#if !defined(GL_ANGLE_texture_usage)
#define GL_TEXTURE_USAGE_ANGLE 0x93A2
#define GL_FRAMEBUFFER_ATTACHMENT_ANGLE 0x93A3
#endif // GL_ANGLE_texture_usage

#if !defined(GL_ANGLE_translated_shader_source)
#define GL_TRANSLATED_SHADER_SOURCE_LENGTH_ANGLE 0x93A0
#endif // GL_ANGLE_translated_shader_source

#if !defined(GL_APPLE_clip_distance)
#define GL_MAX_CLIP_DISTANCES_APPLE 0x0D32
#define GL_CLIP_DISTANCE0_APPLE 0x3000
#define GL_CLIP_DISTANCE1_APPLE 0x3001
#define GL_CLIP_DISTANCE2_APPLE 0x3002
#define GL_CLIP_DISTANCE3_APPLE 0x3003
#define GL_CLIP_DISTANCE4_APPLE 0x3004
#define GL_CLIP_DISTANCE5_APPLE 0x3005
#define GL_CLIP_DISTANCE6_APPLE 0x3006
#define GL_CLIP_DISTANCE7_APPLE 0x3007
#endif // GL_APPLE_clip_distance

#if !defined(GL_APPLE_framebuffer_multisample)
#define GL_RENDERBUFFER_SAMPLES_APPLE 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE 0x8D56
#define GL_MAX_SAMPLES_APPLE 0x8D57
#define GL_READ_FRAMEBUFFER_APPLE 0x8CA8
#define GL_DRAW_FRAMEBUFFER_APPLE 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_APPLE 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_APPLE 0x8CAA
#endif // GL_APPLE_framebuffer_multisample

#if !defined(GL_APPLE_rgb_422)
#define GL_RGB_422_APPLE 0x8A1F
#define GL_UNSIGNED_SHORT_8_8_APPLE 0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_APPLE 0x85BB
#define GL_RGB_RAW_422_APPLE 0x8A51
#endif // GL_APPLE_rgb_422

#if !defined(GL_APPLE_sync)
#define GL_SYNC_OBJECT_APPLE 0x8A53
#define GL_MAX_SERVER_WAIT_TIMEOUT_APPLE 0x9111
#define GL_OBJECT_TYPE_APPLE 0x9112
#define GL_SYNC_CONDITION_APPLE 0x9113
#define GL_SYNC_STATUS_APPLE 0x9114
#define GL_SYNC_FLAGS_APPLE 0x9115
#define GL_SYNC_FENCE_APPLE 0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE_APPLE 0x9117
#define GL_UNSIGNALED_APPLE 0x9118
#define GL_SIGNALED_APPLE 0x9119
#define GL_ALREADY_SIGNALED_APPLE 0x911A
#define GL_TIMEOUT_EXPIRED_APPLE 0x911B
#define GL_CONDITION_SATISFIED_APPLE 0x911C
#define GL_WAIT_FAILED_APPLE 0x911D
#define GL_SYNC_FLUSH_COMMANDS_BIT_APPLE 0x00000001
#define GL_TIMEOUT_IGNORED_APPLE 0xFFFFFFFFFFFFFFFF
#endif // GL_APPLE_sync

#if !defined(GL_APPLE_texture_format_BGRA8888) && !defined(GL_EXT_read_format_bgra) &&                                 \
	!defined(GL_EXT_texture_format_BGRA8888)
#define GL_BGRA_EXT 0x80E1
#endif // GL_APPLE_texture_format_BGRA8888 && GL_EXT_read_format_bgra && GL_EXT_texture_format_BGRA8888

#if !defined(GL_APPLE_texture_format_BGRA8888) && !defined(GL_EXT_texture_storage)
#define GL_BGRA8_EXT 0x93A1
#endif // GL_APPLE_texture_format_BGRA8888 && GL_EXT_texture_storage

#if !defined(GL_APPLE_texture_max_level)
#define GL_TEXTURE_MAX_LEVEL_APPLE 0x813D
#endif // GL_APPLE_texture_max_level

#if !defined(GL_APPLE_texture_packed_float)
#define GL_UNSIGNED_INT_10F_11F_11F_REV_APPLE 0x8C3B
#define GL_UNSIGNED_INT_5_9_9_9_REV_APPLE 0x8C3E
#define GL_R11F_G11F_B10F_APPLE 0x8C3A
#define GL_RGB9_E5_APPLE 0x8C3D
#endif // GL_APPLE_texture_packed_float

#if !defined(GL_ARM_mali_program_binary)
#define GL_MALI_PROGRAM_BINARY_ARM 0x8F61
#endif // GL_ARM_mali_program_binary

#if !defined(GL_ARM_mali_shader_binary)
#define GL_MALI_SHADER_BINARY_ARM 0x8F60
#endif // GL_ARM_mali_shader_binary

#if !defined(GL_ARM_shader_framebuffer_fetch)
#define GL_FETCH_PER_SAMPLE_ARM 0x8F65
#define GL_FRAGMENT_SHADER_FRAMEBUFFER_FETCH_MRT_ARM 0x8F66
#endif // GL_ARM_shader_framebuffer_fetch

#if !defined(GL_DMP_program_binary)
#define GL_SMAPHS30_PROGRAM_BINARY_DMP 0x9251
#define GL_SMAPHS_PROGRAM_BINARY_DMP 0x9252
#define GL_DMP_PROGRAM_BINARY_DMP 0x9253
#endif // GL_DMP_program_binary

#if !defined(GL_DMP_shader_binary)
#define GL_SHADER_BINARY_DMP 0x9250
#endif // GL_DMP_shader_binary

#if !defined(GL_EXT_YUV_target)
#define GL_SAMPLER_EXTERNAL_2D_Y2Y_EXT 0x8BE7
#endif // GL_EXT_YUV_target

#if !defined(GL_EXT_YUV_target) && !defined(GL_OES_EGL_image_external)
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#define GL_TEXTURE_BINDING_EXTERNAL_OES 0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES 0x8D68
#endif // GL_EXT_YUV_target && GL_OES_EGL_image_external

#if !defined(GL_EXT_blend_func_extended)
#define GL_SRC1_COLOR_EXT 0x88F9
#define GL_SRC1_ALPHA_EXT 0x8589
#define GL_ONE_MINUS_SRC1_COLOR_EXT 0x88FA
#define GL_ONE_MINUS_SRC1_ALPHA_EXT 0x88FB
#define GL_SRC_ALPHA_SATURATE_EXT 0x0308
#define GL_LOCATION_INDEX_EXT 0x930F
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS_EXT 0x88FC
#endif // GL_EXT_blend_func_extended

#if !defined(GL_EXT_blend_minmax)
#define GL_MIN_EXT 0x8007
#define GL_MAX_EXT 0x8008
#define GL_FUNC_ADD_EXT 0x8006
#define GL_BLEND_EQUATION_EXT 0x8009
#endif // GL_EXT_blend_minmax

#if !defined(GL_EXT_buffer_storage)
#define GL_MAP_READ_BIT 0x0001
#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_PERSISTENT_BIT_EXT 0x0040
#define GL_MAP_COHERENT_BIT_EXT 0x0080
#define GL_DYNAMIC_STORAGE_BIT_EXT 0x0100
#define GL_CLIENT_STORAGE_BIT_EXT 0x0200
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT_EXT 0x00004000
#define GL_BUFFER_IMMUTABLE_STORAGE_EXT 0x821F
#define GL_BUFFER_STORAGE_FLAGS_EXT 0x8220
#endif // GL_EXT_buffer_storage

#if !defined(GL_EXT_clip_control)
#define GL_LOWER_LEFT_EXT 0x8CA1
#define GL_UPPER_LEFT_EXT 0x8CA2
#define GL_NEGATIVE_ONE_TO_ONE_EXT 0x935E
#define GL_ZERO_TO_ONE_EXT 0x935F
#define GL_CLIP_ORIGIN_EXT 0x935C
#define GL_CLIP_DEPTH_MODE_EXT 0x935D
#endif // GL_EXT_clip_control

#if !defined(GL_EXT_clip_cull_distance)
#define GL_MAX_CLIP_DISTANCES_EXT 0x0D32
#define GL_MAX_CULL_DISTANCES_EXT 0x82F9
#define GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES_EXT 0x82FA
#define GL_CLIP_DISTANCE0_EXT 0x3000
#define GL_CLIP_DISTANCE1_EXT 0x3001
#define GL_CLIP_DISTANCE2_EXT 0x3002
#define GL_CLIP_DISTANCE3_EXT 0x3003
#define GL_CLIP_DISTANCE4_EXT 0x3004
#define GL_CLIP_DISTANCE5_EXT 0x3005
#define GL_CLIP_DISTANCE6_EXT 0x3006
#define GL_CLIP_DISTANCE7_EXT 0x3007
#endif // GL_EXT_clip_cull_distance

#if !defined(GL_EXT_color_buffer_half_float) && !defined(GL_EXT_texture_storage)
#define GL_RGBA16F_EXT 0x881A
#define GL_RGB16F_EXT 0x881B
#define GL_RG16F_EXT 0x822F
#define GL_R16F_EXT 0x822D
#endif // GL_EXT_color_buffer_half_float && GL_EXT_texture_storage

#if !defined(GL_EXT_color_buffer_half_float)
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE_EXT 0x8211
#define GL_UNSIGNED_NORMALIZED_EXT 0x8C17
#endif // GL_EXT_color_buffer_half_float

#if !defined(GL_EXT_debug_label)
#define GL_PROGRAM_PIPELINE_OBJECT_EXT 0x8A4F
#define GL_PROGRAM_OBJECT_EXT 0x8B40
#define GL_SHADER_OBJECT_EXT 0x8B48
#define GL_BUFFER_OBJECT_EXT 0x9151
#define GL_QUERY_OBJECT_EXT 0x9153
#define GL_VERTEX_ARRAY_OBJECT_EXT 0x9154
#define GL_TRANSFORM_FEEDBACK 0x8E22
#endif // GL_EXT_debug_label

#if !defined(GL_EXT_debug_label) && !defined(GL_KHR_debug)
#define GL_SAMPLER 0x82E6
#endif // GL_EXT_debug_label && GL_KHR_debug

#if !defined(GL_EXT_depth_clamp)
#define GL_DEPTH_CLAMP_EXT 0x864F
#endif // GL_EXT_depth_clamp

#if !defined(GL_EXT_discard_framebuffer)
#define GL_COLOR_EXT 0x1800
#define GL_DEPTH_EXT 0x1801
#define GL_STENCIL_EXT 0x1802
#endif // GL_EXT_discard_framebuffer

#if !defined(GL_EXT_disjoint_timer_query)
#define GL_QUERY_COUNTER_BITS_EXT 0x8864
#define GL_TIME_ELAPSED_EXT 0x88BF
#define GL_TIMESTAMP_EXT 0x8E28
#define GL_GPU_DISJOINT_EXT 0x8FBB
#endif // GL_EXT_disjoint_timer_query

#if !defined(GL_EXT_disjoint_timer_query) && !defined(GL_EXT_occlusion_query_boolean)
#define GL_CURRENT_QUERY_EXT 0x8865
#define GL_QUERY_RESULT_EXT 0x8866
#define GL_QUERY_RESULT_AVAILABLE_EXT 0x8867
#endif // GL_EXT_disjoint_timer_query && GL_EXT_occlusion_query_boolean

#if !defined(GL_EXT_draw_buffers)
#define GL_MAX_COLOR_ATTACHMENTS_EXT 0x8CDF
#define GL_MAX_DRAW_BUFFERS_EXT 0x8824
#define GL_DRAW_BUFFER0_EXT 0x8825
#define GL_DRAW_BUFFER1_EXT 0x8826
#define GL_DRAW_BUFFER2_EXT 0x8827
#define GL_DRAW_BUFFER3_EXT 0x8828
#define GL_DRAW_BUFFER4_EXT 0x8829
#define GL_DRAW_BUFFER5_EXT 0x882A
#define GL_DRAW_BUFFER6_EXT 0x882B
#define GL_DRAW_BUFFER7_EXT 0x882C
#define GL_DRAW_BUFFER8_EXT 0x882D
#define GL_DRAW_BUFFER9_EXT 0x882E
#define GL_DRAW_BUFFER10_EXT 0x882F
#define GL_DRAW_BUFFER11_EXT 0x8830
#define GL_DRAW_BUFFER12_EXT 0x8831
#define GL_DRAW_BUFFER13_EXT 0x8832
#define GL_DRAW_BUFFER14_EXT 0x8833
#define GL_DRAW_BUFFER15_EXT 0x8834
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT 0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT 0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT 0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT 0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT 0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT 0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT 0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT 0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT 0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT 0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT 0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT 0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT 0x8CED
#define GL_COLOR_ATTACHMENT14_EXT 0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT 0x8CEF
#endif // GL_EXT_draw_buffers

#if !defined(GL_EXT_draw_buffers_indexed) && !defined(GL_OES_draw_buffers_indexed)
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_BLEND 0x0BE2
#define GL_FUNC_ADD 0x8006
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_MIN 0x8007
#define GL_MAX 0x8008
#define GL_ONE 1
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_SRC_ALPHA_SATURATE 0x0308
#endif // GL_EXT_draw_buffers_indexed && GL_OES_draw_buffers_indexed

#if !defined(GL_EXT_draw_buffers_indexed) && !defined(GL_NV_blend_equation_advanced) &&                                \
	!defined(GL_OES_draw_buffers_indexed)
#define GL_ZERO 0
#endif // GL_EXT_draw_buffers_indexed && GL_NV_blend_equation_advanced && GL_OES_draw_buffers_indexed

#if !defined(GL_EXT_geometry_shader)
#define GL_GEOMETRY_SHADER_EXT 0x8DD9
#define GL_GEOMETRY_SHADER_BIT_EXT 0x00000004
#define GL_GEOMETRY_LINKED_VERTICES_OUT_EXT 0x8916
#define GL_GEOMETRY_LINKED_INPUT_TYPE_EXT 0x8917
#define GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT 0x8918
#define GL_GEOMETRY_SHADER_INVOCATIONS_EXT 0x887F
#define GL_LAYER_PROVOKING_VERTEX_EXT 0x825E
#define GL_LINES_ADJACENCY_EXT 0x000A
#define GL_LINE_STRIP_ADJACENCY_EXT 0x000B
#define GL_TRIANGLES_ADJACENCY_EXT 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_EXT 0x000D
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT 0x8A2C
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8A32
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT 0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT 0x8DE1
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT 0x8E5A
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT 0x8C29
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT 0x92CF
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT 0x92D5
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT 0x90CD
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT 0x90D7
#define GL_FIRST_VERTEX_CONVENTION_EXT 0x8E4D
#define GL_LAST_VERTEX_CONVENTION_EXT 0x8E4E
#define GL_UNDEFINED_VERTEX_EXT 0x8260
#define GL_PRIMITIVES_GENERATED_EXT 0x8C87
#define GL_FRAMEBUFFER_DEFAULT_LAYERS_EXT 0x9312
#define GL_MAX_FRAMEBUFFER_LAYERS_EXT 0x9317
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT 0x8DA8
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT 0x8DA7
#define GL_REFERENCED_BY_GEOMETRY_SHADER_EXT 0x9309
#endif // GL_EXT_geometry_shader

#if !defined(GL_EXT_instanced_arrays)
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_EXT 0x88FE
#endif // GL_EXT_instanced_arrays

#if !defined(GL_EXT_map_buffer_range)
#define GL_MAP_READ_BIT_EXT 0x0001
#define GL_MAP_WRITE_BIT_EXT 0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT_EXT 0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT_EXT 0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT_EXT 0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT_EXT 0x0020
#endif // GL_EXT_map_buffer_range

#if !defined(GL_EXT_memory_object)
#define GL_TEXTURE_TILING_EXT 0x9580
#define GL_DEDICATED_MEMORY_OBJECT_EXT 0x9581
#define GL_PROTECTED_MEMORY_OBJECT_EXT 0x959B
#define GL_NUM_TILING_TYPES_EXT 0x9582
#define GL_TILING_TYPES_EXT 0x9583
#define GL_OPTIMAL_TILING_EXT 0x9584
#define GL_LINEAR_TILING_EXT 0x9585
#endif // GL_EXT_memory_object

#if !defined(GL_EXT_memory_object) && !defined(GL_EXT_semaphore)
#define GL_NUM_DEVICE_UUIDS_EXT 0x9596
#define GL_DEVICE_UUID_EXT 0x9597
#define GL_DRIVER_UUID_EXT 0x9598
#define GL_UUID_SIZE_EXT 16
#endif // GL_EXT_memory_object && GL_EXT_semaphore

#if !defined(GL_EXT_memory_object_fd) && !defined(GL_EXT_semaphore_fd)
#define GL_HANDLE_TYPE_OPAQUE_FD_EXT 0x9586
#endif // GL_EXT_memory_object_fd && GL_EXT_semaphore_fd

#if !defined(GL_EXT_memory_object_win32) && !defined(GL_EXT_semaphore_win32)
#define GL_HANDLE_TYPE_OPAQUE_WIN32_EXT 0x9587
#define GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT 0x9588
#define GL_DEVICE_LUID_EXT 0x9599
#define GL_DEVICE_NODE_MASK_EXT 0x959A
#define GL_LUID_SIZE_EXT 8
#endif // GL_EXT_memory_object_win32 && GL_EXT_semaphore_win32

#if !defined(GL_EXT_memory_object_win32)
#define GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT 0x9589
#define GL_HANDLE_TYPE_D3D12_RESOURCE_EXT 0x958A
#define GL_HANDLE_TYPE_D3D11_IMAGE_EXT 0x958B
#define GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT 0x958C
#endif // GL_EXT_memory_object_win32

#if !defined(GL_EXT_multisampled_compatibility)
#define GL_MULTISAMPLE_EXT 0x809D
#define GL_SAMPLE_ALPHA_TO_ONE_EXT 0x809F
#endif // GL_EXT_multisampled_compatibility

#if !defined(GL_EXT_multisampled_render_to_texture)
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SAMPLES_EXT 0x8D6C
#define GL_RENDERBUFFER_SAMPLES_EXT 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT 0x8D56
#define GL_MAX_SAMPLES_EXT 0x8D57
#endif // GL_EXT_multisampled_render_to_texture

#if !defined(GL_EXT_multiview_draw_buffers)
#define GL_COLOR_ATTACHMENT_EXT 0x90F0
#define GL_MULTIVIEW_EXT 0x90F1
#define GL_DRAW_BUFFER_EXT 0x0C01
#define GL_READ_BUFFER_EXT 0x0C02
#define GL_MAX_MULTIVIEW_BUFFERS_EXT 0x90F2
#endif // GL_EXT_multiview_draw_buffers

#if !defined(GL_EXT_occlusion_query_boolean)
#define GL_ANY_SAMPLES_PASSED_EXT 0x8C2F
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT 0x8D6A
#endif // GL_EXT_occlusion_query_boolean

#if !defined(GL_EXT_polygon_offset_clamp)
#define GL_POLYGON_OFFSET_CLAMP_EXT 0x8E1B
#endif // GL_EXT_polygon_offset_clamp

#if !defined(GL_EXT_primitive_bounding_box)
#define GL_PRIMITIVE_BOUNDING_BOX_EXT 0x92BE
#endif // GL_EXT_primitive_bounding_box

#if !defined(GL_EXT_protected_textures)
#define GL_CONTEXT_FLAG_PROTECTED_CONTENT_BIT_EXT 0x00000010
#define GL_TEXTURE_PROTECTED_EXT 0x8BFA
#endif // GL_EXT_protected_textures

#if !defined(GL_EXT_pvrtc_sRGB)
#define GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT 0x8A55
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT 0x8A56
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT 0x8A57
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG 0x93F0
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG 0x93F1
#endif // GL_EXT_pvrtc_sRGB

#if !defined(GL_EXT_raster_multisample) && !defined(GL_NV_framebuffer_mixed_samples)
#define GL_RASTER_MULTISAMPLE_EXT 0x9327
#define GL_RASTER_SAMPLES_EXT 0x9328
#define GL_MAX_RASTER_SAMPLES_EXT 0x9329
#define GL_RASTER_FIXED_SAMPLE_LOCATIONS_EXT 0x932A
#define GL_MULTISAMPLE_RASTERIZATION_ALLOWED_EXT 0x932B
#define GL_EFFECTIVE_RASTER_SAMPLES_EXT 0x932C
#endif // GL_EXT_raster_multisample && GL_NV_framebuffer_mixed_samples

#if !defined(GL_EXT_read_format_bgra)
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT 0x8366
#endif // GL_EXT_read_format_bgra

#if !defined(GL_EXT_render_snorm)
#define GL_BYTE 0x1400
#define GL_SHORT 0x1402
#define GL_R8_SNORM 0x8F94
#define GL_RG8_SNORM 0x8F95
#define GL_RGBA8_SNORM 0x8F97
#endif // GL_EXT_render_snorm

#if !defined(GL_EXT_render_snorm) && !defined(GL_EXT_texture_norm16)
#define GL_R16_SNORM_EXT 0x8F98
#define GL_RG16_SNORM_EXT 0x8F99
#define GL_RGBA16_SNORM_EXT 0x8F9B
#endif // GL_EXT_render_snorm && GL_EXT_texture_norm16

#if !defined(GL_EXT_robustness) && !defined(GL_KHR_robustness) && !defined(GL_KHR_robustness)
#define GL_NO_ERROR 0
#endif // GL_EXT_robustness && GL_KHR_robustness && GL_KHR_robustness

#if !defined(GL_EXT_robustness)
#define GL_GUILTY_CONTEXT_RESET_EXT 0x8253
#define GL_INNOCENT_CONTEXT_RESET_EXT 0x8254
#define GL_UNKNOWN_CONTEXT_RESET_EXT 0x8255
#define GL_CONTEXT_ROBUST_ACCESS_EXT 0x90F3
#define GL_RESET_NOTIFICATION_STRATEGY_EXT 0x8256
#define GL_LOSE_CONTEXT_ON_RESET_EXT 0x8252
#define GL_NO_RESET_NOTIFICATION_EXT 0x8261
#endif // GL_EXT_robustness

#if !defined(GL_EXT_semaphore)
#define GL_LAYOUT_GENERAL_EXT 0x958D
#define GL_LAYOUT_COLOR_ATTACHMENT_EXT 0x958E
#define GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT 0x958F
#define GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT 0x9590
#define GL_LAYOUT_SHADER_READ_ONLY_EXT 0x9591
#define GL_LAYOUT_TRANSFER_SRC_EXT 0x9592
#define GL_LAYOUT_TRANSFER_DST_EXT 0x9593
#define GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT 0x9530
#define GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT 0x9531
#endif // GL_EXT_semaphore

#if !defined(GL_EXT_semaphore_win32)
#define GL_HANDLE_TYPE_D3D12_FENCE_EXT 0x9594
#define GL_D3D12_FENCE_VALUE_EXT 0x9595
#endif // GL_EXT_semaphore_win32

#if !defined(GL_EXT_sRGB)
#define GL_SRGB_EXT 0x8C40
#define GL_SRGB_ALPHA_EXT 0x8C42
#define GL_SRGB8_ALPHA8_EXT 0x8C43
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_EXT 0x8210
#endif // GL_EXT_sRGB

#if !defined(GL_EXT_sRGB_write_control)
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9
#endif // GL_EXT_sRGB_write_control

#if !defined(GL_EXT_separate_shader_objects) && !defined(GL_EXT_separate_shader_objects)
#define GL_ACTIVE_PROGRAM_EXT 0x8B8D
#endif // GL_EXT_separate_shader_objects && GL_EXT_separate_shader_objects

#if !defined(GL_EXT_separate_shader_objects)
#define GL_VERTEX_SHADER_BIT_EXT 0x00000001
#define GL_FRAGMENT_SHADER_BIT_EXT 0x00000002
#define GL_ALL_SHADER_BITS_EXT 0xFFFFFFFF
#define GL_PROGRAM_SEPARABLE_EXT 0x8258
#define GL_PROGRAM_PIPELINE_BINDING_EXT 0x825A
#endif // GL_EXT_separate_shader_objects

#if !defined(GL_EXT_shader_framebuffer_fetch) && !defined(GL_EXT_shader_framebuffer_fetch_non_coherent)
#define GL_FRAGMENT_SHADER_DISCARDS_SAMPLES_EXT 0x8A52
#endif // GL_EXT_shader_framebuffer_fetch && GL_EXT_shader_framebuffer_fetch_non_coherent

#if !defined(GL_EXT_shader_pixel_local_storage)
#define GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_FAST_SIZE_EXT 0x8F63
#define GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_SIZE_EXT 0x8F67
#define GL_SHADER_PIXEL_LOCAL_STORAGE_EXT 0x8F64
#endif // GL_EXT_shader_pixel_local_storage

#if !defined(GL_EXT_shader_pixel_local_storage2)
#define GL_MAX_SHADER_COMBINED_LOCAL_STORAGE_FAST_SIZE_EXT 0x9650
#define GL_MAX_SHADER_COMBINED_LOCAL_STORAGE_SIZE_EXT 0x9651
#define GL_FRAMEBUFFER_INCOMPLETE_INSUFFICIENT_SHADER_COMBINED_LOCAL_STORAGE_EXT 0x9652
#endif // GL_EXT_shader_pixel_local_storage2

#if !defined(GL_EXT_shadow_samplers)
#define GL_TEXTURE_COMPARE_MODE_EXT 0x884C
#define GL_TEXTURE_COMPARE_FUNC_EXT 0x884D
#define GL_COMPARE_REF_TO_TEXTURE_EXT 0x884E
#define GL_SAMPLER_2D_SHADOW_EXT 0x8B62
#endif // GL_EXT_shadow_samplers

#if !defined(GL_EXT_sparse_texture)
#define GL_TEXTURE_SPARSE_EXT 0x91A6
#define GL_VIRTUAL_PAGE_SIZE_INDEX_EXT 0x91A7
#define GL_NUM_SPARSE_LEVELS_EXT 0x91AA
#define GL_NUM_VIRTUAL_PAGE_SIZES_EXT 0x91A8
#define GL_VIRTUAL_PAGE_SIZE_X_EXT 0x9195
#define GL_VIRTUAL_PAGE_SIZE_Y_EXT 0x9196
#define GL_VIRTUAL_PAGE_SIZE_Z_EXT 0x9197
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_3D 0x806F
#define GL_MAX_SPARSE_TEXTURE_SIZE_EXT 0x9198
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_EXT 0x9199
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_EXT 0x919A
#define GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_EXT 0x91A9
#endif // GL_EXT_sparse_texture

#if !defined(GL_EXT_sparse_texture) && !defined(GL_OES_texture_cube_map_array)
#define GL_TEXTURE_CUBE_MAP_ARRAY_OES 0x9009
#endif // GL_EXT_sparse_texture && GL_OES_texture_cube_map_array

#if !defined(GL_EXT_tessellation_shader)
#define GL_PATCHES_EXT 0x000E
#define GL_PATCH_VERTICES_EXT 0x8E72
#define GL_TESS_CONTROL_OUTPUT_VERTICES_EXT 0x8E75
#define GL_TESS_GEN_MODE_EXT 0x8E76
#define GL_TESS_GEN_SPACING_EXT 0x8E77
#define GL_TESS_GEN_VERTEX_ORDER_EXT 0x8E78
#define GL_TESS_GEN_POINT_MODE_EXT 0x8E79
#define GL_ISOLINES_EXT 0x8E7A
#define GL_QUADS_EXT 0x0007
#define GL_FRACTIONAL_ODD_EXT 0x8E7B
#define GL_FRACTIONAL_EVEN_EXT 0x8E7C
#define GL_MAX_PATCH_VERTICES_EXT 0x8E7D
#define GL_MAX_TESS_GEN_LEVEL_EXT 0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS_EXT 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS_EXT 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS_EXT 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS_EXT 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_EXT 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS_EXT 0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS_EXT 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_EXT 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS_EXT 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS_EXT 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_EXT 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_EXT 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS_EXT 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS_EXT 0x8E1F
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS_EXT 0x92CD
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS_EXT 0x92CE
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS_EXT 0x92D3
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS_EXT 0x92D4
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS_EXT 0x90CB
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS_EXT 0x90CC
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS_EXT 0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS_EXT 0x90D9
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED 0x8221
#define GL_IS_PER_PATCH_EXT 0x92E7
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER_EXT 0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER_EXT 0x9308
#define GL_TESS_CONTROL_SHADER_EXT 0x8E88
#define GL_TESS_EVALUATION_SHADER_EXT 0x8E87
#define GL_TESS_CONTROL_SHADER_BIT_EXT 0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT_EXT 0x00000010
#endif // GL_EXT_tessellation_shader

#if !defined(GL_EXT_tessellation_shader) && !defined(GL_OES_tessellation_shader)
#define GL_TRIANGLES 0x0004
#define GL_EQUAL 0x0202
#define GL_CCW 0x0901
#define GL_CW 0x0900
#endif // GL_EXT_tessellation_shader && GL_OES_tessellation_shader

#if !defined(GL_EXT_texture_border_clamp)
#define GL_TEXTURE_BORDER_COLOR_EXT 0x1004
#define GL_CLAMP_TO_BORDER_EXT 0x812D
#endif // GL_EXT_texture_border_clamp

#if !defined(GL_EXT_texture_buffer)
#define GL_TEXTURE_BUFFER_EXT 0x8C2A
#define GL_TEXTURE_BUFFER_BINDING_EXT 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE_EXT 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER_EXT 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT 0x8C2D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT 0x919F
#define GL_SAMPLER_BUFFER_EXT 0x8DC2
#define GL_INT_SAMPLER_BUFFER_EXT 0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT 0x8DD8
#define GL_IMAGE_BUFFER_EXT 0x9051
#define GL_INT_IMAGE_BUFFER_EXT 0x905C
#define GL_UNSIGNED_INT_IMAGE_BUFFER_EXT 0x9067
#define GL_TEXTURE_BUFFER_OFFSET_EXT 0x919D
#define GL_TEXTURE_BUFFER_SIZE_EXT 0x919E
#endif // GL_EXT_texture_buffer

#if !defined(GL_EXT_texture_compression_astc_decode_mode)
#define GL_TEXTURE_ASTC_DECODE_PRECISION_EXT 0x8F69
#endif // GL_EXT_texture_compression_astc_decode_mode

#if !defined(GL_EXT_texture_compression_bptc)
#define GL_COMPRESSED_RGBA_BPTC_UNORM_EXT 0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT 0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_EXT 0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_EXT 0x8E8F
#endif // GL_EXT_texture_compression_bptc

#if !defined(GL_EXT_texture_compression_dxt1) && !defined(GL_EXT_texture_compression_s3tc)
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif // GL_EXT_texture_compression_dxt1 && GL_EXT_texture_compression_s3tc

#if !defined(GL_EXT_texture_compression_rgtc)
#define GL_COMPRESSED_RED_RGTC1_EXT 0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#endif // GL_EXT_texture_compression_rgtc

#if !defined(GL_EXT_texture_compression_s3tc)
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif // GL_EXT_texture_compression_s3tc

#if !defined(GL_EXT_texture_compression_s3tc_srgb)
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif // GL_EXT_texture_compression_s3tc_srgb

#if !defined(GL_EXT_texture_cube_map_array)
#define GL_TEXTURE_CUBE_MAP_ARRAY_EXT 0x9009
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT 0x900A
#define GL_SAMPLER_CUBE_MAP_ARRAY_EXT 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_EXT 0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_EXT 0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_EXT 0x900F
#define GL_IMAGE_CUBE_MAP_ARRAY_EXT 0x9054
#define GL_INT_IMAGE_CUBE_MAP_ARRAY_EXT 0x905F
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_EXT 0x906A
#endif // GL_EXT_texture_cube_map_array

#if !defined(GL_EXT_texture_filter_anisotropic)
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif // GL_EXT_texture_filter_anisotropic

#if !defined(GL_EXT_texture_filter_minmax)
#define GL_TEXTURE_REDUCTION_MODE_EXT 0x9366
#define GL_WEIGHTED_AVERAGE_EXT 0x9367
#endif // GL_EXT_texture_filter_minmax

#if !defined(GL_EXT_texture_format_sRGB_override)
#define GL_TEXTURE_FORMAT_SRGB_OVERRIDE_EXT 0x8FBF
#endif // GL_EXT_texture_format_sRGB_override

#if !defined(GL_EXT_texture_mirror_clamp_to_edge)
#define GL_MIRROR_CLAMP_TO_EDGE_EXT 0x8743
#endif // GL_EXT_texture_mirror_clamp_to_edge

#if !defined(GL_EXT_texture_norm16)
#define GL_R16_EXT 0x822A
#define GL_RG16_EXT 0x822C
#define GL_RGBA16_EXT 0x805B
#define GL_RGB16_EXT 0x8054
#define GL_RGB16_SNORM_EXT 0x8F9A
#endif // GL_EXT_texture_norm16

#if !defined(GL_EXT_texture_rg)
#define GL_RED_EXT 0x1903
#define GL_RG_EXT 0x8227
#endif // GL_EXT_texture_rg

#if !defined(GL_EXT_texture_rg) && !defined(GL_EXT_texture_storage)
#define GL_R8_EXT 0x8229
#define GL_RG8_EXT 0x822B
#endif // GL_EXT_texture_rg && GL_EXT_texture_storage

#if !defined(GL_EXT_texture_sRGB_R8)
#define GL_SR8_EXT 0x8FBD
#endif // GL_EXT_texture_sRGB_R8

#if !defined(GL_EXT_texture_sRGB_RG8)
#define GL_SRG8_EXT 0x8FBE
#endif // GL_EXT_texture_sRGB_RG8

#if !defined(GL_EXT_texture_sRGB_decode)
#define GL_TEXTURE_SRGB_DECODE_EXT 0x8A48
#define GL_DECODE_EXT 0x8A49
#define GL_SKIP_DECODE_EXT 0x8A4A
#endif // GL_EXT_texture_sRGB_decode

#if !defined(GL_EXT_texture_storage)
#define GL_TEXTURE_IMMUTABLE_FORMAT_EXT 0x912F
#define GL_ALPHA8_EXT 0x803C
#define GL_LUMINANCE8_EXT 0x8040
#define GL_LUMINANCE8_ALPHA8_EXT 0x8045
#define GL_RGBA32F_EXT 0x8814
#define GL_RGB32F_EXT 0x8815
#define GL_ALPHA32F_EXT 0x8816
#define GL_LUMINANCE32F_EXT 0x8818
#define GL_LUMINANCE_ALPHA32F_EXT 0x8819
#define GL_ALPHA16F_EXT 0x881C
#define GL_LUMINANCE16F_EXT 0x881E
#define GL_LUMINANCE_ALPHA16F_EXT 0x881F
#define GL_R32F_EXT 0x822E
#define GL_RG32F_EXT 0x8230
#endif // GL_EXT_texture_storage

#if !defined(GL_EXT_texture_storage) && !defined(GL_OES_required_internalformat)
#define GL_RGB10_A2_EXT 0x8059
#define GL_RGB10_EXT 0x8052
#endif // GL_EXT_texture_storage && GL_OES_required_internalformat

#if !defined(GL_EXT_texture_type_2_10_10_10_REV)
#define GL_UNSIGNED_INT_2_10_10_10_REV_EXT 0x8368
#endif // GL_EXT_texture_type_2_10_10_10_REV

#if !defined(GL_EXT_texture_view)
#define GL_TEXTURE_VIEW_MIN_LEVEL_EXT 0x82DB
#define GL_TEXTURE_VIEW_NUM_LEVELS_EXT 0x82DC
#define GL_TEXTURE_VIEW_MIN_LAYER_EXT 0x82DD
#define GL_TEXTURE_VIEW_NUM_LAYERS_EXT 0x82DE
#endif // GL_EXT_texture_view

#if !defined(GL_EXT_texture_view) && !defined(GL_OES_texture_view)
#define GL_TEXTURE_IMMUTABLE_LEVELS 0x82DF
#endif // GL_EXT_texture_view && GL_OES_texture_view

#if !defined(GL_EXT_unpack_subimage)
#define GL_UNPACK_ROW_LENGTH_EXT 0x0CF2
#define GL_UNPACK_SKIP_ROWS_EXT 0x0CF3
#define GL_UNPACK_SKIP_PIXELS_EXT 0x0CF4
#endif // GL_EXT_unpack_subimage

#if !defined(GL_EXT_window_rectangles)
#define GL_INCLUSIVE_EXT 0x8F10
#define GL_EXCLUSIVE_EXT 0x8F11
#define GL_WINDOW_RECTANGLE_EXT 0x8F12
#define GL_WINDOW_RECTANGLE_MODE_EXT 0x8F13
#define GL_MAX_WINDOW_RECTANGLES_EXT 0x8F14
#define GL_NUM_WINDOW_RECTANGLES_EXT 0x8F15
#endif // GL_EXT_window_rectangles

#if !defined(GL_FJ_shader_binary_GCCSO)
#define GL_GCCSO_SHADER_BINARY_FJ 0x9260
#endif // GL_FJ_shader_binary_GCCSO

#if !defined(GL_IMG_framebuffer_downsample)
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_AND_DOWNSAMPLE_IMG 0x913C
#define GL_NUM_DOWNSAMPLE_SCALES_IMG 0x913D
#define GL_DOWNSAMPLE_SCALES_IMG 0x913E
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SCALE_IMG 0x913F
#endif // GL_IMG_framebuffer_downsample

#if !defined(GL_IMG_multisampled_render_to_texture)
#define GL_RENDERBUFFER_SAMPLES_IMG 0x9133
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_IMG 0x9134
#define GL_MAX_SAMPLES_IMG 0x9135
#define GL_TEXTURE_SAMPLES_IMG 0x9136
#endif // GL_IMG_multisampled_render_to_texture

#if !defined(GL_IMG_program_binary)
#define GL_SGX_PROGRAM_BINARY_IMG 0x9130
#endif // GL_IMG_program_binary

#if !defined(GL_IMG_read_format)
#define GL_BGRA_IMG 0x80E1
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG 0x8365
#endif // GL_IMG_read_format

#if !defined(GL_IMG_shader_binary)
#define GL_SGX_BINARY_IMG 0x8C0A
#endif // GL_IMG_shader_binary

#if !defined(GL_IMG_texture_compression_pvrtc)
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif // GL_IMG_texture_compression_pvrtc

#if !defined(GL_IMG_texture_compression_pvrtc2)
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG 0x9138
#endif // GL_IMG_texture_compression_pvrtc2

#if !defined(GL_IMG_texture_filter_cubic)
#define GL_CUBIC_IMG 0x9139
#define GL_CUBIC_MIPMAP_NEAREST_IMG 0x913A
#define GL_CUBIC_MIPMAP_LINEAR_IMG 0x913B
#endif // GL_IMG_texture_filter_cubic

#if !defined(GL_INTEL_conservative_rasterization)
#define GL_CONSERVATIVE_RASTERIZATION_INTEL 0x83FE
#endif // GL_INTEL_conservative_rasterization

#if !defined(GL_INTEL_blackhole_render)
#define GL_BLACKHOLE_RENDER_INTEL 0x83FC
#endif // GL_INTEL_blackhole_render

#if !defined(GL_INTEL_performance_query)
#define GL_PERFQUERY_SINGLE_CONTEXT_INTEL 0x00000000
#define GL_PERFQUERY_GLOBAL_CONTEXT_INTEL 0x00000001
#define GL_PERFQUERY_WAIT_INTEL 0x83FB
#define GL_PERFQUERY_FLUSH_INTEL 0x83FA
#define GL_PERFQUERY_DONOT_FLUSH_INTEL 0x83F9
#define GL_PERFQUERY_COUNTER_EVENT_INTEL 0x94F0
#define GL_PERFQUERY_COUNTER_DURATION_NORM_INTEL 0x94F1
#define GL_PERFQUERY_COUNTER_DURATION_RAW_INTEL 0x94F2
#define GL_PERFQUERY_COUNTER_THROUGHPUT_INTEL 0x94F3
#define GL_PERFQUERY_COUNTER_RAW_INTEL 0x94F4
#define GL_PERFQUERY_COUNTER_TIMESTAMP_INTEL 0x94F5
#define GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL 0x94F8
#define GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL 0x94F9
#define GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL 0x94FA
#define GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL 0x94FB
#define GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL 0x94FC
#define GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL 0x94FD
#define GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL 0x94FE
#define GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL 0x94FF
#define GL_PERFQUERY_GPA_EXTENDED_COUNTERS_INTEL 0x9500
#endif // GL_INTEL_performance_query

#if !defined(GL_KHR_blend_equation_advanced)
#define GL_MULTIPLY_KHR 0x9294
#define GL_SCREEN_KHR 0x9295
#define GL_OVERLAY_KHR 0x9296
#define GL_DARKEN_KHR 0x9297
#define GL_LIGHTEN_KHR 0x9298
#define GL_COLORDODGE_KHR 0x9299
#define GL_COLORBURN_KHR 0x929A
#define GL_HARDLIGHT_KHR 0x929B
#define GL_SOFTLIGHT_KHR 0x929C
#define GL_DIFFERENCE_KHR 0x929E
#define GL_EXCLUSION_KHR 0x92A0
#define GL_HSL_HUE_KHR 0x92AD
#define GL_HSL_SATURATION_KHR 0x92AE
#define GL_HSL_COLOR_KHR 0x92AF
#define GL_HSL_LUMINOSITY_KHR 0x92B0
#endif // GL_KHR_blend_equation_advanced

#if !defined(GL_KHR_blend_equation_advanced_coherent)
#define GL_BLEND_ADVANCED_COHERENT_KHR 0x9285
#endif // GL_KHR_blend_equation_advanced_coherent

#if !defined(GL_KHR_context_flush_control)
#define GL_CONTEXT_RELEASE_BEHAVIOR 0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH 0x82FC
#define GL_CONTEXT_RELEASE_BEHAVIOR_KHR 0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_KHR 0x82FC
#endif // GL_KHR_context_flush_control

#if !defined(GL_KHR_context_flush_control) && !defined(GL_KHR_context_flush_control)
#define GL_NONE 0
#endif // GL_KHR_context_flush_control && GL_KHR_context_flush_control

#if !defined(GL_KHR_debug)
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM 0x8245
#define GL_DEBUG_SOURCE_API 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#define GL_DEBUG_SOURCE_OTHER 0x824B
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DEBUG_TYPE_MARKER 0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP 0x8269
#define GL_DEBUG_TYPE_POP_GROUP 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH 0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH 0x826D
#define GL_BUFFER 0x82E0
#define GL_SHADER 0x82E1
#define GL_PROGRAM 0x82E2
#define GL_VERTEX_ARRAY 0x8074
#define GL_QUERY 0x82E3
#define GL_PROGRAM_PIPELINE 0x82E4
#define GL_MAX_LABEL_LENGTH 0x82E8
#define GL_MAX_DEBUG_MESSAGE_LENGTH 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES 0x9144
#define GL_DEBUG_LOGGED_MESSAGES 0x9145
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_KHR 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION_KHR 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM_KHR 0x8245
#define GL_DEBUG_SOURCE_API_KHR 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_KHR 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_KHR 0x8249
#define GL_DEBUG_SOURCE_APPLICATION_KHR 0x824A
#define GL_DEBUG_SOURCE_OTHER_KHR 0x824B
#define GL_DEBUG_TYPE_ERROR_KHR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY_KHR 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_KHR 0x8250
#define GL_DEBUG_TYPE_OTHER_KHR 0x8251
#define GL_DEBUG_TYPE_MARKER_KHR 0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP_KHR 0x8269
#define GL_DEBUG_TYPE_POP_GROUP_KHR 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION_KHR 0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH_KHR 0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH_KHR 0x826D
#define GL_BUFFER_KHR 0x82E0
#define GL_SHADER_KHR 0x82E1
#define GL_PROGRAM_KHR 0x82E2
#define GL_VERTEX_ARRAY_KHR 0x8074
#define GL_QUERY_KHR 0x82E3
#define GL_PROGRAM_PIPELINE_KHR 0x82E4
#define GL_SAMPLER_KHR 0x82E6
#define GL_MAX_LABEL_LENGTH_KHR 0x82E8
#define GL_MAX_DEBUG_MESSAGE_LENGTH_KHR 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_KHR 0x9144
#define GL_DEBUG_LOGGED_MESSAGES_KHR 0x9145
#define GL_DEBUG_SEVERITY_HIGH_KHR 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_KHR 0x9147
#define GL_DEBUG_SEVERITY_LOW_KHR 0x9148
#define GL_DEBUG_OUTPUT_KHR 0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT_KHR 0x00000002
#define GL_STACK_OVERFLOW_KHR 0x0503
#define GL_STACK_UNDERFLOW_KHR 0x0504
#define GL_DISPLAY_LIST 0x82E7
#endif // GL_KHR_debug

#if !defined(GL_KHR_no_error)
#define GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR 0x00000008
#endif // GL_KHR_no_error

#if !defined(GL_KHR_robustness)
#define GL_CONTEXT_ROBUST_ACCESS 0x90F3
#define GL_LOSE_CONTEXT_ON_RESET 0x8252
#define GL_GUILTY_CONTEXT_RESET 0x8253
#define GL_INNOCENT_CONTEXT_RESET 0x8254
#define GL_UNKNOWN_CONTEXT_RESET 0x8255
#define GL_RESET_NOTIFICATION_STRATEGY 0x8256
#define GL_NO_RESET_NOTIFICATION 0x8261
#define GL_CONTEXT_LOST 0x0507
#define GL_CONTEXT_ROBUST_ACCESS_KHR 0x90F3
#define GL_LOSE_CONTEXT_ON_RESET_KHR 0x8252
#define GL_GUILTY_CONTEXT_RESET_KHR 0x8253
#define GL_INNOCENT_CONTEXT_RESET_KHR 0x8254
#define GL_UNKNOWN_CONTEXT_RESET_KHR 0x8255
#define GL_RESET_NOTIFICATION_STRATEGY_KHR 0x8256
#define GL_NO_RESET_NOTIFICATION_KHR 0x8261
#define GL_CONTEXT_LOST_KHR 0x0507
#endif // GL_KHR_robustness

#if !defined(GL_KHR_texture_compression_astc_hdr) && !defined(GL_KHR_texture_compression_astc_ldr) &&                  \
	!defined(GL_OES_texture_compression_astc)
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR 0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR 0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR 0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR 0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR 0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR 0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR 0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR 0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR 0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR 0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR 0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR 0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR 0x93BD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR 0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR 0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR 0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR 0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR 0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR 0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR 0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR 0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR 0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR 0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR 0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR 0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR 0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR 0x93DD
#endif // GL_KHR_texture_compression_astc_hdr && GL_KHR_texture_compression_astc_ldr && GL_OES_texture_compression_astc

#if !defined(GL_KHR_parallel_shader_compile)
#define GL_MAX_SHADER_COMPILER_THREADS_KHR 0x91B0
#define GL_COMPLETION_STATUS_KHR 0x91B1
#endif // GL_KHR_parallel_shader_compile

#if !defined(GL_MESA_framebuffer_flip_y)
#define GL_FRAMEBUFFER_FLIP_Y_MESA 0x8BBB
#endif // GL_MESA_framebuffer_flip_y

#if !defined(GL_MESA_program_binary_formats)
#define GL_PROGRAM_BINARY_FORMAT_MESA 0x875F
#endif // GL_MESA_program_binary_formats

#if !defined(GL_NV_blend_equation_advanced)
#define GL_BLEND_OVERLAP_NV 0x9281
#define GL_BLEND_PREMULTIPLIED_SRC_NV 0x9280
#define GL_BLUE_NV 0x1905
#define GL_COLORBURN_NV 0x929A
#define GL_COLORDODGE_NV 0x9299
#define GL_CONJOINT_NV 0x9284
#define GL_CONTRAST_NV 0x92A1
#define GL_DARKEN_NV 0x9297
#define GL_DIFFERENCE_NV 0x929E
#define GL_DISJOINT_NV 0x9283
#define GL_DST_ATOP_NV 0x928F
#define GL_DST_IN_NV 0x928B
#define GL_DST_NV 0x9287
#define GL_DST_OUT_NV 0x928D
#define GL_DST_OVER_NV 0x9289
#define GL_EXCLUSION_NV 0x92A0
#define GL_GREEN_NV 0x1904
#define GL_HARDLIGHT_NV 0x929B
#define GL_HARDMIX_NV 0x92A9
#define GL_HSL_COLOR_NV 0x92AF
#define GL_HSL_HUE_NV 0x92AD
#define GL_HSL_LUMINOSITY_NV 0x92B0
#define GL_HSL_SATURATION_NV 0x92AE
#define GL_INVERT 0x150A
#define GL_INVERT_OVG_NV 0x92B4
#define GL_INVERT_RGB_NV 0x92A3
#define GL_LIGHTEN_NV 0x9298
#define GL_LINEARBURN_NV 0x92A5
#define GL_LINEARDODGE_NV 0x92A4
#define GL_LINEARLIGHT_NV 0x92A7
#define GL_MINUS_CLAMPED_NV 0x92B3
#define GL_MINUS_NV 0x929F
#define GL_MULTIPLY_NV 0x9294
#define GL_OVERLAY_NV 0x9296
#define GL_PINLIGHT_NV 0x92A8
#define GL_PLUS_CLAMPED_ALPHA_NV 0x92B2
#define GL_PLUS_CLAMPED_NV 0x92B1
#define GL_PLUS_DARKER_NV 0x9292
#define GL_PLUS_NV 0x9291
#define GL_RED_NV 0x1903
#define GL_SCREEN_NV 0x9295
#define GL_SOFTLIGHT_NV 0x929C
#define GL_SRC_ATOP_NV 0x928E
#define GL_SRC_IN_NV 0x928A
#define GL_SRC_NV 0x9286
#define GL_SRC_OUT_NV 0x928C
#define GL_SRC_OVER_NV 0x9288
#define GL_UNCORRELATED_NV 0x9282
#define GL_VIVIDLIGHT_NV 0x92A6
#define GL_XOR_NV 0x1506
#endif // GL_NV_blend_equation_advanced

#if !defined(GL_NV_blend_equation_advanced_coherent)
#define GL_BLEND_ADVANCED_COHERENT_NV 0x9285
#endif // GL_NV_blend_equation_advanced_coherent

#if !defined(GL_NV_blend_minmax_factor)
#define GL_FACTOR_MIN_AMD 0x901C
#define GL_FACTOR_MAX_AMD 0x901D
#endif // GL_NV_blend_minmax_factor

#if !defined(GL_NV_clip_space_w_scaling)
#define GL_VIEWPORT_POSITION_W_SCALE_NV 0x937C
#define GL_VIEWPORT_POSITION_W_SCALE_X_COEFF_NV 0x937D
#define GL_VIEWPORT_POSITION_W_SCALE_Y_COEFF_NV 0x937E
#endif // GL_NV_clip_space_w_scaling

#if !defined(GL_NV_conditional_render)
#define GL_QUERY_WAIT_NV 0x8E13
#define GL_QUERY_NO_WAIT_NV 0x8E14
#define GL_QUERY_BY_REGION_WAIT_NV 0x8E15
#define GL_QUERY_BY_REGION_NO_WAIT_NV 0x8E16
#endif // GL_NV_conditional_render

#if !defined(GL_NV_conservative_raster)
#define GL_CONSERVATIVE_RASTERIZATION_NV 0x9346
#define GL_SUBPIXEL_PRECISION_BIAS_X_BITS_NV 0x9347
#define GL_SUBPIXEL_PRECISION_BIAS_Y_BITS_NV 0x9348
#define GL_MAX_SUBPIXEL_PRECISION_BIAS_BITS_NV 0x9349
#endif // GL_NV_conservative_raster

#if !defined(GL_NV_conservative_raster_pre_snap)
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_NV 0x9550
#endif // GL_NV_conservative_raster_pre_snap

#if !defined(GL_NV_conservative_raster_pre_snap_triangles) && !defined(GL_NV_conservative_raster_pre_snap_triangles)
#define GL_CONSERVATIVE_RASTER_MODE_NV 0x954D
#endif // GL_NV_conservative_raster_pre_snap_triangles && GL_NV_conservative_raster_pre_snap_triangles

#if !defined(GL_NV_conservative_raster_pre_snap_triangles)
#define GL_CONSERVATIVE_RASTER_MODE_POST_SNAP_NV 0x954E
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_TRIANGLES_NV 0x954F
#endif // GL_NV_conservative_raster_pre_snap_triangles

#if !defined(GL_NV_copy_buffer)
#define GL_COPY_READ_BUFFER_NV 0x8F36
#define GL_COPY_WRITE_BUFFER_NV 0x8F37
#endif // GL_NV_copy_buffer

#if !defined(GL_NV_coverage_sample)
#define GL_COVERAGE_COMPONENT_NV 0x8ED0
#define GL_COVERAGE_COMPONENT4_NV 0x8ED1
#define GL_COVERAGE_ATTACHMENT_NV 0x8ED2
#define GL_COVERAGE_BUFFERS_NV 0x8ED3
#define GL_COVERAGE_SAMPLES_NV 0x8ED4
#define GL_COVERAGE_ALL_FRAGMENTS_NV 0x8ED5
#define GL_COVERAGE_EDGE_FRAGMENTS_NV 0x8ED6
#define GL_COVERAGE_AUTOMATIC_NV 0x8ED7
#define GL_COVERAGE_BUFFER_BIT_NV 0x00008000
#endif // GL_NV_coverage_sample

#if !defined(GL_NV_depth_nonlinear)
#define GL_DEPTH_COMPONENT16_NONLINEAR_NV 0x8E2C
#endif // GL_NV_depth_nonlinear

#if !defined(GL_NV_draw_buffers)
#define GL_MAX_DRAW_BUFFERS_NV 0x8824
#define GL_DRAW_BUFFER0_NV 0x8825
#define GL_DRAW_BUFFER1_NV 0x8826
#define GL_DRAW_BUFFER2_NV 0x8827
#define GL_DRAW_BUFFER3_NV 0x8828
#define GL_DRAW_BUFFER4_NV 0x8829
#define GL_DRAW_BUFFER5_NV 0x882A
#define GL_DRAW_BUFFER6_NV 0x882B
#define GL_DRAW_BUFFER7_NV 0x882C
#define GL_DRAW_BUFFER8_NV 0x882D
#define GL_DRAW_BUFFER9_NV 0x882E
#define GL_DRAW_BUFFER10_NV 0x882F
#define GL_DRAW_BUFFER11_NV 0x8830
#define GL_DRAW_BUFFER12_NV 0x8831
#define GL_DRAW_BUFFER13_NV 0x8832
#define GL_DRAW_BUFFER14_NV 0x8833
#define GL_DRAW_BUFFER15_NV 0x8834
#endif // GL_NV_draw_buffers

#if !defined(GL_NV_draw_buffers) && !defined(GL_NV_fbo_color_attachments)
#define GL_COLOR_ATTACHMENT0_NV 0x8CE0
#define GL_COLOR_ATTACHMENT1_NV 0x8CE1
#define GL_COLOR_ATTACHMENT2_NV 0x8CE2
#define GL_COLOR_ATTACHMENT3_NV 0x8CE3
#define GL_COLOR_ATTACHMENT4_NV 0x8CE4
#define GL_COLOR_ATTACHMENT5_NV 0x8CE5
#define GL_COLOR_ATTACHMENT6_NV 0x8CE6
#define GL_COLOR_ATTACHMENT7_NV 0x8CE7
#define GL_COLOR_ATTACHMENT8_NV 0x8CE8
#define GL_COLOR_ATTACHMENT9_NV 0x8CE9
#define GL_COLOR_ATTACHMENT10_NV 0x8CEA
#define GL_COLOR_ATTACHMENT11_NV 0x8CEB
#define GL_COLOR_ATTACHMENT12_NV 0x8CEC
#define GL_COLOR_ATTACHMENT13_NV 0x8CED
#define GL_COLOR_ATTACHMENT14_NV 0x8CEE
#define GL_COLOR_ATTACHMENT15_NV 0x8CEF
#endif // GL_NV_draw_buffers && GL_NV_fbo_color_attachments

#if !defined(GL_NV_fbo_color_attachments)
#define GL_MAX_COLOR_ATTACHMENTS_NV 0x8CDF
#endif // GL_NV_fbo_color_attachments

#if !defined(GL_NV_fence)
#define GL_ALL_COMPLETED_NV 0x84F2
#define GL_FENCE_STATUS_NV 0x84F3
#define GL_FENCE_CONDITION_NV 0x84F4
#endif // GL_NV_fence

#if !defined(GL_NV_fill_rectangle)
#define GL_FILL_RECTANGLE_NV 0x933C
#endif // GL_NV_fill_rectangle

#if !defined(GL_NV_fragment_coverage_to_color)
#define GL_FRAGMENT_COVERAGE_TO_COLOR_NV 0x92DD
#define GL_FRAGMENT_COVERAGE_COLOR_NV 0x92DE
#endif // GL_NV_fragment_coverage_to_color

#if !defined(GL_NV_framebuffer_blit)
#define GL_READ_FRAMEBUFFER_NV 0x8CA8
#define GL_DRAW_FRAMEBUFFER_NV 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_NV 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_NV 0x8CAA
#endif // GL_NV_framebuffer_blit

#if !defined(GL_NV_framebuffer_mixed_samples)
#define GL_COVERAGE_MODULATION_TABLE_NV 0x9331
#define GL_COLOR_SAMPLES_NV 0x8E20
#define GL_DEPTH_SAMPLES_NV 0x932D
#define GL_STENCIL_SAMPLES_NV 0x932E
#define GL_MIXED_DEPTH_SAMPLES_SUPPORTED_NV 0x932F
#define GL_MIXED_STENCIL_SAMPLES_SUPPORTED_NV 0x9330
#define GL_COVERAGE_MODULATION_NV 0x9332
#define GL_COVERAGE_MODULATION_TABLE_SIZE_NV 0x9333
#endif // GL_NV_framebuffer_mixed_samples

#if !defined(GL_NV_framebuffer_multisample)
#define GL_RENDERBUFFER_SAMPLES_NV 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_NV 0x8D56
#define GL_MAX_SAMPLES_NV 0x8D57
#endif // GL_NV_framebuffer_multisample

#if !defined(GL_NV_gpu_shader5)
#define GL_INT64_NV 0x140E
#define GL_UNSIGNED_INT64_NV 0x140F
#define GL_INT8_NV 0x8FE0
#define GL_INT8_VEC2_NV 0x8FE1
#define GL_INT8_VEC3_NV 0x8FE2
#define GL_INT8_VEC4_NV 0x8FE3
#define GL_INT16_NV 0x8FE4
#define GL_INT16_VEC2_NV 0x8FE5
#define GL_INT16_VEC3_NV 0x8FE6
#define GL_INT16_VEC4_NV 0x8FE7
#define GL_INT64_VEC2_NV 0x8FE9
#define GL_INT64_VEC3_NV 0x8FEA
#define GL_INT64_VEC4_NV 0x8FEB
#define GL_UNSIGNED_INT8_NV 0x8FEC
#define GL_UNSIGNED_INT8_VEC2_NV 0x8FED
#define GL_UNSIGNED_INT8_VEC3_NV 0x8FEE
#define GL_UNSIGNED_INT8_VEC4_NV 0x8FEF
#define GL_UNSIGNED_INT16_NV 0x8FF0
#define GL_UNSIGNED_INT16_VEC2_NV 0x8FF1
#define GL_UNSIGNED_INT16_VEC3_NV 0x8FF2
#define GL_UNSIGNED_INT16_VEC4_NV 0x8FF3
#define GL_UNSIGNED_INT64_VEC2_NV 0x8FF5
#define GL_UNSIGNED_INT64_VEC3_NV 0x8FF6
#define GL_UNSIGNED_INT64_VEC4_NV 0x8FF7
#define GL_FLOAT16_NV 0x8FF8
#define GL_FLOAT16_VEC2_NV 0x8FF9
#define GL_FLOAT16_VEC3_NV 0x8FFA
#define GL_FLOAT16_VEC4_NV 0x8FFB
#define GL_PATCHES 0x000E
#endif // GL_NV_gpu_shader5

#if !defined(GL_NV_instanced_arrays)
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_NV 0x88FE
#endif // GL_NV_instanced_arrays

#if !defined(GL_NV_internalformat_sample_query)
#define GL_RENDERBUFFER 0x8D41
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#define GL_MULTISAMPLES_NV 0x9371
#define GL_SUPERSAMPLE_SCALE_X_NV 0x9372
#define GL_SUPERSAMPLE_SCALE_Y_NV 0x9373
#define GL_CONFORMANT_NV 0x9374
#endif // GL_NV_internalformat_sample_query

#if !defined(GL_NV_memory_attachment)
#define GL_ATTACHED_MEMORY_OBJECT_NV 0x95A4
#define GL_ATTACHED_MEMORY_OFFSET_NV 0x95A5
#define GL_MEMORY_ATTACHABLE_ALIGNMENT_NV 0x95A6
#define GL_MEMORY_ATTACHABLE_SIZE_NV 0x95A7
#define GL_MEMORY_ATTACHABLE_NV 0x95A8
#define GL_DETACHED_MEMORY_INCARNATION_NV 0x95A9
#define GL_DETACHED_TEXTURES_NV 0x95AA
#define GL_DETACHED_BUFFERS_NV 0x95AB
#define GL_MAX_DETACHED_TEXTURES_NV 0x95AC
#define GL_MAX_DETACHED_BUFFERS_NV 0x95AD
#endif // GL_NV_memory_attachment

#if !defined(GL_NV_mesh_shader)
#define GL_MESH_SHADER_NV 0x9559
#define GL_TASK_SHADER_NV 0x955A
#define GL_MAX_MESH_UNIFORM_BLOCKS_NV 0x8E60
#define GL_MAX_MESH_TEXTURE_IMAGE_UNITS_NV 0x8E61
#define GL_MAX_MESH_IMAGE_UNIFORMS_NV 0x8E62
#define GL_MAX_MESH_UNIFORM_COMPONENTS_NV 0x8E63
#define GL_MAX_MESH_ATOMIC_COUNTER_BUFFERS_NV 0x8E64
#define GL_MAX_MESH_ATOMIC_COUNTERS_NV 0x8E65
#define GL_MAX_MESH_SHADER_STORAGE_BLOCKS_NV 0x8E66
#define GL_MAX_COMBINED_MESH_UNIFORM_COMPONENTS_NV 0x8E67
#define GL_MAX_TASK_UNIFORM_BLOCKS_NV 0x8E68
#define GL_MAX_TASK_TEXTURE_IMAGE_UNITS_NV 0x8E69
#define GL_MAX_TASK_IMAGE_UNIFORMS_NV 0x8E6A
#define GL_MAX_TASK_UNIFORM_COMPONENTS_NV 0x8E6B
#define GL_MAX_TASK_ATOMIC_COUNTER_BUFFERS_NV 0x8E6C
#define GL_MAX_TASK_ATOMIC_COUNTERS_NV 0x8E6D
#define GL_MAX_TASK_SHADER_STORAGE_BLOCKS_NV 0x8E6E
#define GL_MAX_COMBINED_TASK_UNIFORM_COMPONENTS_NV 0x8E6F
#define GL_MAX_MESH_WORK_GROUP_INVOCATIONS_NV 0x95A2
#define GL_MAX_TASK_WORK_GROUP_INVOCATIONS_NV 0x95A3
#define GL_MAX_MESH_TOTAL_MEMORY_SIZE_NV 0x9536
#define GL_MAX_TASK_TOTAL_MEMORY_SIZE_NV 0x9537
#define GL_MAX_MESH_OUTPUT_VERTICES_NV 0x9538
#define GL_MAX_MESH_OUTPUT_PRIMITIVES_NV 0x9539
#define GL_MAX_TASK_OUTPUT_COUNT_NV 0x953A
#define GL_MAX_DRAW_MESH_TASKS_COUNT_NV 0x953D
#define GL_MAX_MESH_VIEWS_NV 0x9557
#define GL_MESH_OUTPUT_PER_VERTEX_GRANULARITY_NV 0x92DF
#define GL_MESH_OUTPUT_PER_PRIMITIVE_GRANULARITY_NV 0x9543
#define GL_MAX_MESH_WORK_GROUP_SIZE_NV 0x953B
#define GL_MAX_TASK_WORK_GROUP_SIZE_NV 0x953C
#define GL_MESH_WORK_GROUP_SIZE_NV 0x953E
#define GL_TASK_WORK_GROUP_SIZE_NV 0x953F
#define GL_MESH_VERTICES_OUT_NV 0x9579
#define GL_MESH_PRIMITIVES_OUT_NV 0x957A
#define GL_MESH_OUTPUT_TYPE_NV 0x957B
#define GL_UNIFORM_BLOCK_REFERENCED_BY_MESH_SHADER_NV 0x959C
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TASK_SHADER_NV 0x959D
#define GL_REFERENCED_BY_MESH_SHADER_NV 0x95A0
#define GL_REFERENCED_BY_TASK_SHADER_NV 0x95A1
#define GL_MESH_SHADER_BIT_NV 0x00000040
#define GL_TASK_SHADER_BIT_NV 0x00000080
#define GL_MESH_SUBROUTINE_NV 0x957C
#define GL_TASK_SUBROUTINE_NV 0x957D
#define GL_MESH_SUBROUTINE_UNIFORM_NV 0x957E
#define GL_TASK_SUBROUTINE_UNIFORM_NV 0x957F
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_MESH_SHADER_NV 0x959E
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TASK_SHADER_NV 0x959F
#endif // GL_NV_mesh_shader

#if !defined(GL_NV_non_square_matrices)
#define GL_FLOAT_MAT2x3_NV 0x8B65
#define GL_FLOAT_MAT2x4_NV 0x8B66
#define GL_FLOAT_MAT3x2_NV 0x8B67
#define GL_FLOAT_MAT3x4_NV 0x8B68
#define GL_FLOAT_MAT4x2_NV 0x8B69
#define GL_FLOAT_MAT4x3_NV 0x8B6A
#endif // GL_NV_non_square_matrices

#if !defined(GL_NV_path_rendering)
#define GL_PATH_FORMAT_SVG_NV 0x9070
#define GL_PATH_FORMAT_PS_NV 0x9071
#define GL_STANDARD_FONT_NAME_NV 0x9072
#define GL_SYSTEM_FONT_NAME_NV 0x9073
#define GL_FILE_NAME_NV 0x9074
#define GL_PATH_STROKE_WIDTH_NV 0x9075
#define GL_PATH_END_CAPS_NV 0x9076
#define GL_PATH_INITIAL_END_CAP_NV 0x9077
#define GL_PATH_TERMINAL_END_CAP_NV 0x9078
#define GL_PATH_JOIN_STYLE_NV 0x9079
#define GL_PATH_MITER_LIMIT_NV 0x907A
#define GL_PATH_DASH_CAPS_NV 0x907B
#define GL_PATH_INITIAL_DASH_CAP_NV 0x907C
#define GL_PATH_TERMINAL_DASH_CAP_NV 0x907D
#define GL_PATH_DASH_OFFSET_NV 0x907E
#define GL_PATH_CLIENT_LENGTH_NV 0x907F
#define GL_PATH_FILL_MODE_NV 0x9080
#define GL_PATH_FILL_MASK_NV 0x9081
#define GL_PATH_FILL_COVER_MODE_NV 0x9082
#define GL_PATH_STROKE_COVER_MODE_NV 0x9083
#define GL_PATH_STROKE_MASK_NV 0x9084
#define GL_COUNT_UP_NV 0x9088
#define GL_COUNT_DOWN_NV 0x9089
#define GL_PATH_OBJECT_BOUNDING_BOX_NV 0x908A
#define GL_CONVEX_HULL_NV 0x908B
#define GL_BOUNDING_BOX_NV 0x908D
#define GL_TRANSLATE_X_NV 0x908E
#define GL_TRANSLATE_Y_NV 0x908F
#define GL_TRANSLATE_2D_NV 0x9090
#define GL_TRANSLATE_3D_NV 0x9091
#define GL_AFFINE_2D_NV 0x9092
#define GL_AFFINE_3D_NV 0x9094
#define GL_TRANSPOSE_AFFINE_2D_NV 0x9096
#define GL_TRANSPOSE_AFFINE_3D_NV 0x9098
#define GL_UTF8_NV 0x909A
#define GL_UTF16_NV 0x909B
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV 0x909C
#define GL_PATH_COMMAND_COUNT_NV 0x909D
#define GL_PATH_COORD_COUNT_NV 0x909E
#define GL_PATH_DASH_ARRAY_COUNT_NV 0x909F
#define GL_PATH_COMPUTED_LENGTH_NV 0x90A0
#define GL_PATH_FILL_BOUNDING_BOX_NV 0x90A1
#define GL_PATH_STROKE_BOUNDING_BOX_NV 0x90A2
#define GL_SQUARE_NV 0x90A3
#define GL_ROUND_NV 0x90A4
#define GL_TRIANGULAR_NV 0x90A5
#define GL_BEVEL_NV 0x90A6
#define GL_MITER_REVERT_NV 0x90A7
#define GL_MITER_TRUNCATE_NV 0x90A8
#define GL_SKIP_MISSING_GLYPH_NV 0x90A9
#define GL_USE_MISSING_GLYPH_NV 0x90AA
#define GL_PATH_ERROR_POSITION_NV 0x90AB
#define GL_ACCUM_ADJACENT_PAIRS_NV 0x90AD
#define GL_ADJACENT_PAIRS_NV 0x90AE
#define GL_FIRST_TO_REST_NV 0x90AF
#define GL_PATH_GEN_MODE_NV 0x90B0
#define GL_PATH_GEN_COEFF_NV 0x90B1
#define GL_PATH_GEN_COMPONENTS_NV 0x90B3
#define GL_PATH_STENCIL_FUNC_NV 0x90B7
#define GL_PATH_STENCIL_REF_NV 0x90B8
#define GL_PATH_STENCIL_VALUE_MASK_NV 0x90B9
#define GL_PATH_STENCIL_DEPTH_OFFSET_FACTOR_NV 0x90BD
#define GL_PATH_STENCIL_DEPTH_OFFSET_UNITS_NV 0x90BE
#define GL_PATH_COVER_DEPTH_FUNC_NV 0x90BF
#define GL_PATH_DASH_OFFSET_RESET_NV 0x90B4
#define GL_MOVE_TO_RESETS_NV 0x90B5
#define GL_MOVE_TO_CONTINUES_NV 0x90B6
#define GL_CLOSE_PATH_NV 0x00
#define GL_MOVE_TO_NV 0x02
#define GL_RELATIVE_MOVE_TO_NV 0x03
#define GL_LINE_TO_NV 0x04
#define GL_RELATIVE_LINE_TO_NV 0x05
#define GL_HORIZONTAL_LINE_TO_NV 0x06
#define GL_RELATIVE_HORIZONTAL_LINE_TO_NV 0x07
#define GL_VERTICAL_LINE_TO_NV 0x08
#define GL_RELATIVE_VERTICAL_LINE_TO_NV 0x09
#define GL_QUADRATIC_CURVE_TO_NV 0x0A
#define GL_RELATIVE_QUADRATIC_CURVE_TO_NV 0x0B
#define GL_CUBIC_CURVE_TO_NV 0x0C
#define GL_RELATIVE_CUBIC_CURVE_TO_NV 0x0D
#define GL_SMOOTH_QUADRATIC_CURVE_TO_NV 0x0E
#define GL_RELATIVE_SMOOTH_QUADRATIC_CURVE_TO_NV 0x0F
#define GL_SMOOTH_CUBIC_CURVE_TO_NV 0x10
#define GL_RELATIVE_SMOOTH_CUBIC_CURVE_TO_NV 0x11
#define GL_SMALL_CCW_ARC_TO_NV 0x12
#define GL_RELATIVE_SMALL_CCW_ARC_TO_NV 0x13
#define GL_SMALL_CW_ARC_TO_NV 0x14
#define GL_RELATIVE_SMALL_CW_ARC_TO_NV 0x15
#define GL_LARGE_CCW_ARC_TO_NV 0x16
#define GL_RELATIVE_LARGE_CCW_ARC_TO_NV 0x17
#define GL_LARGE_CW_ARC_TO_NV 0x18
#define GL_RELATIVE_LARGE_CW_ARC_TO_NV 0x19
#define GL_RESTART_PATH_NV 0xF0
#define GL_DUP_FIRST_CUBIC_CURVE_TO_NV 0xF2
#define GL_DUP_LAST_CUBIC_CURVE_TO_NV 0xF4
#define GL_RECT_NV 0xF6
#define GL_CIRCULAR_CCW_ARC_TO_NV 0xF8
#define GL_CIRCULAR_CW_ARC_TO_NV 0xFA
#define GL_CIRCULAR_TANGENT_ARC_TO_NV 0xFC
#define GL_ARC_TO_NV 0xFE
#define GL_RELATIVE_ARC_TO_NV 0xFF
#define GL_BOLD_BIT_NV 0x01
#define GL_ITALIC_BIT_NV 0x02
#define GL_GLYPH_WIDTH_BIT_NV 0x01
#define GL_GLYPH_HEIGHT_BIT_NV 0x02
#define GL_GLYPH_HORIZONTAL_BEARING_X_BIT_NV 0x04
#define GL_GLYPH_HORIZONTAL_BEARING_Y_BIT_NV 0x08
#define GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV 0x10
#define GL_GLYPH_VERTICAL_BEARING_X_BIT_NV 0x20
#define GL_GLYPH_VERTICAL_BEARING_Y_BIT_NV 0x40
#define GL_GLYPH_VERTICAL_BEARING_ADVANCE_BIT_NV 0x80
#define GL_GLYPH_HAS_KERNING_BIT_NV 0x100
#define GL_FONT_X_MIN_BOUNDS_BIT_NV 0x00010000
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV 0x00020000
#define GL_FONT_X_MAX_BOUNDS_BIT_NV 0x00040000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV 0x00080000
#define GL_FONT_UNITS_PER_EM_BIT_NV 0x00100000
#define GL_FONT_ASCENDER_BIT_NV 0x00200000
#define GL_FONT_DESCENDER_BIT_NV 0x00400000
#define GL_FONT_HEIGHT_BIT_NV 0x00800000
#define GL_FONT_MAX_ADVANCE_WIDTH_BIT_NV 0x01000000
#define GL_FONT_MAX_ADVANCE_HEIGHT_BIT_NV 0x02000000
#define GL_FONT_UNDERLINE_POSITION_BIT_NV 0x04000000
#define GL_FONT_UNDERLINE_THICKNESS_BIT_NV 0x08000000
#define GL_FONT_HAS_KERNING_BIT_NV 0x10000000
#define GL_ROUNDED_RECT_NV 0xE8
#define GL_RELATIVE_ROUNDED_RECT_NV 0xE9
#define GL_ROUNDED_RECT2_NV 0xEA
#define GL_RELATIVE_ROUNDED_RECT2_NV 0xEB
#define GL_ROUNDED_RECT4_NV 0xEC
#define GL_RELATIVE_ROUNDED_RECT4_NV 0xED
#define GL_ROUNDED_RECT8_NV 0xEE
#define GL_RELATIVE_ROUNDED_RECT8_NV 0xEF
#define GL_RELATIVE_RECT_NV 0xF7
#define GL_FONT_GLYPHS_AVAILABLE_NV 0x9368
#define GL_FONT_TARGET_UNAVAILABLE_NV 0x9369
#define GL_FONT_UNAVAILABLE_NV 0x936A
#define GL_FONT_UNINTELLIGIBLE_NV 0x936B
#define GL_CONIC_CURVE_TO_NV 0x1A
#define GL_RELATIVE_CONIC_CURVE_TO_NV 0x1B
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV 0x20000000
#define GL_STANDARD_FONT_FORMAT_NV 0x936C
#define GL_2_BYTES_NV 0x1407
#define GL_3_BYTES_NV 0x1408
#define GL_4_BYTES_NV 0x1409
#define GL_EYE_LINEAR_NV 0x2400
#define GL_OBJECT_LINEAR_NV 0x2401
#define GL_CONSTANT_NV 0x8576
#define GL_PATH_FOG_GEN_MODE_NV 0x90AC
#define GL_PRIMARY_COLOR 0x8577
#define GL_PRIMARY_COLOR_NV 0x852C
#define GL_SECONDARY_COLOR_NV 0x852D
#define GL_PATH_GEN_COLOR_FORMAT_NV 0x90B2
#define GL_PATH_PROJECTION_NV 0x1701
#define GL_PATH_MODELVIEW_NV 0x1700
#define GL_PATH_MODELVIEW_STACK_DEPTH_NV 0x0BA3
#define GL_PATH_MODELVIEW_MATRIX_NV 0x0BA6
#define GL_PATH_MAX_MODELVIEW_STACK_DEPTH_NV 0x0D36
#define GL_PATH_TRANSPOSE_MODELVIEW_MATRIX_NV 0x84E3
#define GL_PATH_PROJECTION_STACK_DEPTH_NV 0x0BA4
#define GL_PATH_PROJECTION_MATRIX_NV 0x0BA7
#define GL_PATH_MAX_PROJECTION_STACK_DEPTH_NV 0x0D38
#define GL_PATH_TRANSPOSE_PROJECTION_MATRIX_NV 0x84E4
#define GL_FRAGMENT_INPUT_NV 0x936D
#endif // GL_NV_path_rendering

#if !defined(GL_NV_path_rendering_shared_edge)
#define GL_SHARED_EDGE_NV 0xC0
#endif // GL_NV_path_rendering_shared_edge

#if !defined(GL_NV_pixel_buffer_object)
#define GL_PIXEL_PACK_BUFFER_NV 0x88EB
#define GL_PIXEL_UNPACK_BUFFER_NV 0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING_NV 0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING_NV 0x88EF
#endif // GL_NV_pixel_buffer_object

#if !defined(GL_NV_polygon_mode)
#define GL_POLYGON_MODE_NV 0x0B40
#define GL_POLYGON_OFFSET_POINT_NV 0x2A01
#define GL_POLYGON_OFFSET_LINE_NV 0x2A02
#define GL_POINT_NV 0x1B00
#define GL_LINE_NV 0x1B01
#define GL_FILL_NV 0x1B02
#endif // GL_NV_polygon_mode

#if !defined(GL_NV_read_buffer)
#define GL_READ_BUFFER_NV 0x0C02
#endif // GL_NV_read_buffer

#if !defined(GL_NV_representative_fragment_test)
#define GL_REPRESENTATIVE_FRAGMENT_TEST_NV 0x937F
#endif // GL_NV_representative_fragment_test

#if !defined(GL_NV_sRGB_formats)
#define GL_SLUMINANCE_NV 0x8C46
#define GL_SLUMINANCE_ALPHA_NV 0x8C44
#define GL_SRGB8_NV 0x8C41
#define GL_SLUMINANCE8_NV 0x8C47
#define GL_SLUMINANCE8_ALPHA8_NV 0x8C45
#define GL_COMPRESSED_SRGB_S3TC_DXT1_NV 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_NV 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_NV 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_NV 0x8C4F
#define GL_ETC1_SRGB8_NV 0x88EE
#endif // GL_NV_sRGB_formats

#if !defined(GL_NV_sample_locations)
#define GL_SAMPLE_LOCATION_SUBPIXEL_BITS_NV 0x933D
#define GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_NV 0x933E
#define GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_NV 0x933F
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_NV 0x9340
#define GL_SAMPLE_LOCATION_NV 0x8E50
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_NV 0x9341
#define GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_NV 0x9342
#define GL_FRAMEBUFFER_SAMPLE_LOCATION_PIXEL_GRID_NV 0x9343
#endif // GL_NV_sample_locations

#if !defined(GL_NV_scissor_exclusive)
#define GL_SCISSOR_TEST_EXCLUSIVE_NV 0x9555
#define GL_SCISSOR_BOX_EXCLUSIVE_NV 0x9556
#endif // GL_NV_scissor_exclusive

#if !defined(GL_NV_shading_rate_image)
#define GL_SHADING_RATE_IMAGE_NV 0x9563
#define GL_SHADING_RATE_NO_INVOCATIONS_NV 0x9564
#define GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV 0x9565
#define GL_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV 0x9566
#define GL_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV 0x9567
#define GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV 0x9568
#define GL_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV 0x9569
#define GL_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV 0x956A
#define GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV 0x956B
#define GL_SHADING_RATE_2_INVOCATIONS_PER_PIXEL_NV 0x956C
#define GL_SHADING_RATE_4_INVOCATIONS_PER_PIXEL_NV 0x956D
#define GL_SHADING_RATE_8_INVOCATIONS_PER_PIXEL_NV 0x956E
#define GL_SHADING_RATE_16_INVOCATIONS_PER_PIXEL_NV 0x956F
#define GL_SHADING_RATE_IMAGE_BINDING_NV 0x955B
#define GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV 0x955C
#define GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV 0x955D
#define GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV 0x955E
#define GL_MAX_COARSE_FRAGMENT_SAMPLES_NV 0x955F
#define GL_SHADING_RATE_SAMPLE_ORDER_DEFAULT_NV 0x95AE
#define GL_SHADING_RATE_SAMPLE_ORDER_PIXEL_MAJOR_NV 0x95AF
#define GL_SHADING_RATE_SAMPLE_ORDER_SAMPLE_MAJOR_NV 0x95B0
#endif // GL_NV_shading_rate_image

#if !defined(GL_NV_shadow_samplers_array)
#define GL_SAMPLER_2D_ARRAY_SHADOW_NV 0x8DC4
#endif // GL_NV_shadow_samplers_array

#if !defined(GL_NV_shadow_samplers_cube)
#define GL_SAMPLER_CUBE_SHADOW_NV 0x8DC5
#endif // GL_NV_shadow_samplers_cube

#if !defined(GL_NV_texture_border_clamp)
#define GL_TEXTURE_BORDER_COLOR_NV 0x1004
#define GL_CLAMP_TO_BORDER_NV 0x812D
#endif // GL_NV_texture_border_clamp

#if !defined(GL_NV_viewport_array)
#define GL_MAX_VIEWPORTS_NV 0x825B
#define GL_VIEWPORT_SUBPIXEL_BITS_NV 0x825C
#define GL_VIEWPORT_BOUNDS_RANGE_NV 0x825D
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX_NV 0x825F
#endif // GL_NV_viewport_array

#if !defined(GL_NV_viewport_array) && !defined(GL_OES_viewport_array)
#define GL_SCISSOR_BOX 0x0C10
#define GL_VIEWPORT 0x0BA2
#define GL_DEPTH_RANGE 0x0B70
#define GL_SCISSOR_TEST 0x0C11
#endif // GL_NV_viewport_array && GL_OES_viewport_array

#if !defined(GL_NV_viewport_swizzle)
#define GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV 0x9350
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV 0x9351
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV 0x9352
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV 0x9353
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV 0x9354
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV 0x9355
#define GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV 0x9356
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_W_NV 0x9357
#define GL_VIEWPORT_SWIZZLE_X_NV 0x9358
#define GL_VIEWPORT_SWIZZLE_Y_NV 0x9359
#define GL_VIEWPORT_SWIZZLE_Z_NV 0x935A
#define GL_VIEWPORT_SWIZZLE_W_NV 0x935B
#endif // GL_NV_viewport_swizzle

#if !defined(GL_OES_EGL_image_external)
#define GL_SAMPLER_EXTERNAL_OES 0x8D66
#endif // GL_OES_EGL_image_external

#if !defined(GL_OES_compressed_ETC1_RGB8_texture)
#define GL_ETC1_RGB8_OES 0x8D64
#endif // GL_OES_compressed_ETC1_RGB8_texture

#if !defined(GL_OES_compressed_paletted_texture)
#define GL_PALETTE4_RGB8_OES 0x8B90
#define GL_PALETTE4_RGBA8_OES 0x8B91
#define GL_PALETTE4_R5_G6_B5_OES 0x8B92
#define GL_PALETTE4_RGBA4_OES 0x8B93
#define GL_PALETTE4_RGB5_A1_OES 0x8B94
#define GL_PALETTE8_RGB8_OES 0x8B95
#define GL_PALETTE8_RGBA8_OES 0x8B96
#define GL_PALETTE8_R5_G6_B5_OES 0x8B97
#define GL_PALETTE8_RGBA4_OES 0x8B98
#define GL_PALETTE8_RGB5_A1_OES 0x8B99
#endif // GL_OES_compressed_paletted_texture

#if !defined(GL_OES_depth24) && !defined(GL_OES_required_internalformat)
#define GL_DEPTH_COMPONENT24_OES 0x81A6
#endif // GL_OES_depth24 && GL_OES_required_internalformat

#if !defined(GL_OES_geometry_shader)
#define GL_GEOMETRY_SHADER_OES 0x8DD9
#define GL_GEOMETRY_SHADER_BIT_OES 0x00000004
#define GL_GEOMETRY_LINKED_VERTICES_OUT_OES 0x8916
#define GL_GEOMETRY_LINKED_INPUT_TYPE_OES 0x8917
#define GL_GEOMETRY_LINKED_OUTPUT_TYPE_OES 0x8918
#define GL_GEOMETRY_SHADER_INVOCATIONS_OES 0x887F
#define GL_LAYER_PROVOKING_VERTEX_OES 0x825E
#define GL_LINES_ADJACENCY_OES 0x000A
#define GL_LINE_STRIP_ADJACENCY_OES 0x000B
#define GL_TRIANGLES_ADJACENCY_OES 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_OES 0x000D
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_OES 0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS_OES 0x8A2C
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_OES 0x8A32
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS_OES 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_OES 0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_OES 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_OES 0x8DE1
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS_OES 0x8E5A
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_OES 0x8C29
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_OES 0x92CF
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS_OES 0x92D5
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS_OES 0x90CD
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_OES 0x90D7
#define GL_FIRST_VERTEX_CONVENTION_OES 0x8E4D
#define GL_LAST_VERTEX_CONVENTION_OES 0x8E4E
#define GL_UNDEFINED_VERTEX_OES 0x8260
#define GL_PRIMITIVES_GENERATED_OES 0x8C87
#define GL_FRAMEBUFFER_DEFAULT_LAYERS_OES 0x9312
#define GL_MAX_FRAMEBUFFER_LAYERS_OES 0x9317
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_OES 0x8DA8
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_OES 0x8DA7
#define GL_REFERENCED_BY_GEOMETRY_SHADER_OES 0x9309
#endif // GL_OES_geometry_shader

#if !defined(GL_OES_get_program_binary)
#define GL_PROGRAM_BINARY_LENGTH_OES 0x8741
#define GL_NUM_PROGRAM_BINARY_FORMATS_OES 0x87FE
#define GL_PROGRAM_BINARY_FORMATS_OES 0x87FF
#endif // GL_OES_get_program_binary

#if !defined(GL_OES_mapbuffer)
#define GL_WRITE_ONLY_OES 0x88B9
#define GL_BUFFER_ACCESS_OES 0x88BB
#define GL_BUFFER_MAPPED_OES 0x88BC
#define GL_BUFFER_MAP_POINTER_OES 0x88BD
#endif // GL_OES_mapbuffer

#if !defined(GL_OES_primitive_bounding_box)
#define GL_PRIMITIVE_BOUNDING_BOX_OES 0x92BE
#endif // GL_OES_primitive_bounding_box

#if !defined(GL_OES_required_internalformat)
#define GL_ALPHA8_OES 0x803C
#define GL_DEPTH_COMPONENT16_OES 0x81A5
#define GL_LUMINANCE4_ALPHA4_OES 0x8043
#define GL_LUMINANCE8_ALPHA8_OES 0x8045
#define GL_LUMINANCE8_OES 0x8040
#define GL_RGBA4_OES 0x8056
#define GL_RGB5_A1_OES 0x8057
#define GL_RGB565_OES 0x8D62
#endif // GL_OES_required_internalformat

#if !defined(GL_OES_required_internalformat) && !defined(GL_OES_rgb8_rgba8)
#define GL_RGB8_OES 0x8051
#define GL_RGBA8_OES 0x8058
#endif // GL_OES_required_internalformat && GL_OES_rgb8_rgba8

#if !defined(GL_OES_sample_shading)
#define GL_SAMPLE_SHADING_OES 0x8C36
#define GL_MIN_SAMPLE_SHADING_VALUE_OES 0x8C37
#endif // GL_OES_sample_shading

#if !defined(GL_OES_shader_multisample_interpolation)
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET_OES 0x8E5B
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET_OES 0x8E5C
#define GL_FRAGMENT_INTERPOLATION_OFFSET_BITS_OES 0x8E5D
#endif // GL_OES_shader_multisample_interpolation

#if !defined(GL_OES_standard_derivatives)
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES 0x8B8B
#endif // GL_OES_standard_derivatives

#if !defined(GL_OES_stencil1)
#define GL_STENCIL_INDEX1_OES 0x8D46
#endif // GL_OES_stencil1

#if !defined(GL_OES_stencil4)
#define GL_STENCIL_INDEX4_OES 0x8D47
#endif // GL_OES_stencil4

#if !defined(GL_OES_surfaceless_context)
#define GL_FRAMEBUFFER_UNDEFINED_OES 0x8219
#endif // GL_OES_surfaceless_context

#if !defined(GL_OES_tessellation_shader)
#define GL_PATCHES_OES 0x000E
#define GL_PATCH_VERTICES_OES 0x8E72
#define GL_TESS_CONTROL_OUTPUT_VERTICES_OES 0x8E75
#define GL_TESS_GEN_MODE_OES 0x8E76
#define GL_TESS_GEN_SPACING_OES 0x8E77
#define GL_TESS_GEN_VERTEX_ORDER_OES 0x8E78
#define GL_TESS_GEN_POINT_MODE_OES 0x8E79
#define GL_ISOLINES_OES 0x8E7A
#define GL_QUADS_OES 0x0007
#define GL_FRACTIONAL_ODD_OES 0x8E7B
#define GL_FRACTIONAL_EVEN_OES 0x8E7C
#define GL_MAX_PATCH_VERTICES_OES 0x8E7D
#define GL_MAX_TESS_GEN_LEVEL_OES 0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS_OES 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS_OES 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS_OES 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS_OES 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_OES 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS_OES 0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS_OES 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_OES 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS_OES 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS_OES 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_OES 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_OES 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS_OES 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS_OES 0x8E1F
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS_OES 0x92CD
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS_OES 0x92CE
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS_OES 0x92D3
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS_OES 0x92D4
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS_OES 0x90CB
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS_OES 0x90CC
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS_OES 0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS_OES 0x90D9
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED_OES 0x8221
#define GL_IS_PER_PATCH_OES 0x92E7
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER_OES 0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER_OES 0x9308
#define GL_TESS_CONTROL_SHADER_OES 0x8E88
#define GL_TESS_EVALUATION_SHADER_OES 0x8E87
#define GL_TESS_CONTROL_SHADER_BIT_OES 0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT_OES 0x00000010
#endif // GL_OES_tessellation_shader

#if !defined(GL_OES_texture_3D)
#define GL_TEXTURE_WRAP_R_OES 0x8072
#define GL_TEXTURE_3D_OES 0x806F
#define GL_TEXTURE_BINDING_3D_OES 0x806A
#define GL_MAX_3D_TEXTURE_SIZE_OES 0x8073
#define GL_SAMPLER_3D_OES 0x8B5F
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_OES 0x8CD4
#endif // GL_OES_texture_3D

#if !defined(GL_OES_texture_border_clamp)
#define GL_TEXTURE_BORDER_COLOR_OES 0x1004
#define GL_CLAMP_TO_BORDER_OES 0x812D
#endif // GL_OES_texture_border_clamp

#if !defined(GL_OES_texture_buffer)
#define GL_TEXTURE_BUFFER_OES 0x8C2A
#define GL_TEXTURE_BUFFER_BINDING_OES 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE_OES 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER_OES 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_OES 0x8C2D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_OES 0x919F
#define GL_SAMPLER_BUFFER_OES 0x8DC2
#define GL_INT_SAMPLER_BUFFER_OES 0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_OES 0x8DD8
#define GL_IMAGE_BUFFER_OES 0x9051
#define GL_INT_IMAGE_BUFFER_OES 0x905C
#define GL_UNSIGNED_INT_IMAGE_BUFFER_OES 0x9067
#define GL_TEXTURE_BUFFER_OFFSET_OES 0x919D
#define GL_TEXTURE_BUFFER_SIZE_OES 0x919E
#endif // GL_OES_texture_buffer

#if !defined(GL_OES_texture_compression_astc)
#define GL_COMPRESSED_RGBA_ASTC_3x3x3_OES 0x93C0
#define GL_COMPRESSED_RGBA_ASTC_4x3x3_OES 0x93C1
#define GL_COMPRESSED_RGBA_ASTC_4x4x3_OES 0x93C2
#define GL_COMPRESSED_RGBA_ASTC_4x4x4_OES 0x93C3
#define GL_COMPRESSED_RGBA_ASTC_5x4x4_OES 0x93C4
#define GL_COMPRESSED_RGBA_ASTC_5x5x4_OES 0x93C5
#define GL_COMPRESSED_RGBA_ASTC_5x5x5_OES 0x93C6
#define GL_COMPRESSED_RGBA_ASTC_6x5x5_OES 0x93C7
#define GL_COMPRESSED_RGBA_ASTC_6x6x5_OES 0x93C8
#define GL_COMPRESSED_RGBA_ASTC_6x6x6_OES 0x93C9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES 0x93E0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES 0x93E1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES 0x93E2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES 0x93E3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES 0x93E4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES 0x93E5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES 0x93E6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES 0x93E7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES 0x93E8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES 0x93E9
#endif // GL_OES_texture_compression_astc

#if !defined(GL_OES_texture_cube_map_array)
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_OES 0x900A
#define GL_SAMPLER_CUBE_MAP_ARRAY_OES 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_OES 0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_OES 0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_OES 0x900F
#define GL_IMAGE_CUBE_MAP_ARRAY_OES 0x9054
#define GL_INT_IMAGE_CUBE_MAP_ARRAY_OES 0x905F
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_OES 0x906A
#endif // GL_OES_texture_cube_map_array

#if !defined(GL_OES_texture_float)
#define GL_FLOAT 0x1406
#endif // GL_OES_texture_float

#if !defined(GL_OES_texture_half_float) && !defined(GL_OES_vertex_half_float)
#define GL_HALF_FLOAT_OES 0x8D61
#endif // GL_OES_texture_half_float && GL_OES_vertex_half_float

#if !defined(GL_OES_texture_stencil8)
#define GL_STENCIL_INDEX_OES 0x1901
#define GL_STENCIL_INDEX8_OES 0x8D48
#endif // GL_OES_texture_stencil8

#if !defined(GL_OES_texture_storage_multisample_2d_array)
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES 0x9102
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES 0x9105
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY_OES 0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES 0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES 0x910D
#endif // GL_OES_texture_storage_multisample_2d_array

#if !defined(GL_OES_texture_view)
#define GL_TEXTURE_VIEW_MIN_LEVEL_OES 0x82DB
#define GL_TEXTURE_VIEW_NUM_LEVELS_OES 0x82DC
#define GL_TEXTURE_VIEW_MIN_LAYER_OES 0x82DD
#define GL_TEXTURE_VIEW_NUM_LAYERS_OES 0x82DE
#endif // GL_OES_texture_view

#if !defined(GL_OES_vertex_array_object)
#define GL_VERTEX_ARRAY_BINDING_OES 0x85B5
#endif // GL_OES_vertex_array_object

#if !defined(GL_OES_vertex_type_10_10_10_2)
#define GL_UNSIGNED_INT_10_10_10_2_OES 0x8DF6
#define GL_INT_10_10_10_2_OES 0x8DF7
#endif // GL_OES_vertex_type_10_10_10_2

#if !defined(GL_OES_viewport_array)
#define GL_MAX_VIEWPORTS_OES 0x825B
#define GL_VIEWPORT_SUBPIXEL_BITS_OES 0x825C
#define GL_VIEWPORT_BOUNDS_RANGE_OES 0x825D
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX_OES 0x825F
#endif // GL_OES_viewport_array

#if !defined(GL_OVR_multiview)
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR 0x9630
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR 0x9632
#define GL_MAX_VIEWS_OVR 0x9631
#define GL_FRAMEBUFFER_INCOMPLETE_VIEW_TARGETS_OVR 0x9633
#endif // GL_OVR_multiview

#if !defined(GL_QCOM_alpha_test)
#define GL_ALPHA_TEST_QCOM 0x0BC0
#define GL_ALPHA_TEST_FUNC_QCOM 0x0BC1
#define GL_ALPHA_TEST_REF_QCOM 0x0BC2
#endif // GL_QCOM_alpha_test

#if !defined(GL_QCOM_binning_control)
#define GL_BINNING_CONTROL_HINT_QCOM 0x8FB0
#define GL_CPU_OPTIMIZED_QCOM 0x8FB1
#define GL_GPU_OPTIMIZED_QCOM 0x8FB2
#define GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM 0x8FB3
#endif // GL_QCOM_binning_control

#if !defined(GL_QCOM_extended_get)
#define GL_TEXTURE_WIDTH_QCOM 0x8BD2
#define GL_TEXTURE_HEIGHT_QCOM 0x8BD3
#define GL_TEXTURE_DEPTH_QCOM 0x8BD4
#define GL_TEXTURE_INTERNAL_FORMAT_QCOM 0x8BD5
#define GL_TEXTURE_FORMAT_QCOM 0x8BD6
#define GL_TEXTURE_TYPE_QCOM 0x8BD7
#define GL_TEXTURE_IMAGE_VALID_QCOM 0x8BD8
#define GL_TEXTURE_NUM_LEVELS_QCOM 0x8BD9
#define GL_TEXTURE_TARGET_QCOM 0x8BDA
#define GL_TEXTURE_OBJECT_VALID_QCOM 0x8BDB
#define GL_STATE_RESTORE 0x8BDC
#endif // GL_QCOM_extended_get

#if !defined(GL_QCOM_framebuffer_foveated) && !defined(GL_QCOM_texture_foveated)
#define GL_FOVEATION_ENABLE_BIT_QCOM 0x00000001
#define GL_FOVEATION_SCALED_BIN_METHOD_BIT_QCOM 0x00000002
#endif // GL_QCOM_framebuffer_foveated && GL_QCOM_texture_foveated

#if !defined(GL_QCOM_texture_foveated)
#define GL_TEXTURE_FOVEATED_FEATURE_BITS_QCOM 0x8BFB
#define GL_TEXTURE_FOVEATED_MIN_PIXEL_DENSITY_QCOM 0x8BFC
#define GL_TEXTURE_FOVEATED_FEATURE_QUERY_QCOM 0x8BFD
#define GL_TEXTURE_FOVEATED_NUM_FOCAL_POINTS_QUERY_QCOM 0x8BFE
#define GL_FRAMEBUFFER_INCOMPLETE_FOVEATION_QCOM 0x8BFF
#endif // GL_QCOM_texture_foveated

#if !defined(GL_QCOM_texture_foveated_subsampled_layout)
#define GL_FOVEATION_SUBSAMPLED_LAYOUT_METHOD_BIT_QCOM 0x00000004
#define GL_MAX_SHADER_SUBSAMPLED_IMAGE_UNITS_QCOM 0x8FA1
#endif // GL_QCOM_texture_foveated_subsampled_layout

#if !defined(GL_QCOM_perfmon_global_mode)
#define GL_PERFMON_GLOBAL_MODE_QCOM 0x8FA0
#endif // GL_QCOM_perfmon_global_mode

#if !defined(GL_QCOM_shader_framebuffer_fetch_noncoherent)
#define GL_FRAMEBUFFER_FETCH_NONCOHERENT_QCOM 0x96A2
#endif // GL_QCOM_shader_framebuffer_fetch_noncoherent

#if !defined(GL_QCOM_tiled_rendering)
#define GL_COLOR_BUFFER_BIT0_QCOM 0x00000001
#define GL_COLOR_BUFFER_BIT1_QCOM 0x00000002
#define GL_COLOR_BUFFER_BIT2_QCOM 0x00000004
#define GL_COLOR_BUFFER_BIT3_QCOM 0x00000008
#define GL_COLOR_BUFFER_BIT4_QCOM 0x00000010
#define GL_COLOR_BUFFER_BIT5_QCOM 0x00000020
#define GL_COLOR_BUFFER_BIT6_QCOM 0x00000040
#define GL_COLOR_BUFFER_BIT7_QCOM 0x00000080
#define GL_DEPTH_BUFFER_BIT0_QCOM 0x00000100
#define GL_DEPTH_BUFFER_BIT1_QCOM 0x00000200
#define GL_DEPTH_BUFFER_BIT2_QCOM 0x00000400
#define GL_DEPTH_BUFFER_BIT3_QCOM 0x00000800
#define GL_DEPTH_BUFFER_BIT4_QCOM 0x00001000
#define GL_DEPTH_BUFFER_BIT5_QCOM 0x00002000
#define GL_DEPTH_BUFFER_BIT6_QCOM 0x00004000
#define GL_DEPTH_BUFFER_BIT7_QCOM 0x00008000
#define GL_STENCIL_BUFFER_BIT0_QCOM 0x00010000
#define GL_STENCIL_BUFFER_BIT1_QCOM 0x00020000
#define GL_STENCIL_BUFFER_BIT2_QCOM 0x00040000
#define GL_STENCIL_BUFFER_BIT3_QCOM 0x00080000
#define GL_STENCIL_BUFFER_BIT4_QCOM 0x00100000
#define GL_STENCIL_BUFFER_BIT5_QCOM 0x00200000
#define GL_STENCIL_BUFFER_BIT6_QCOM 0x00400000
#define GL_STENCIL_BUFFER_BIT7_QCOM 0x00800000
#define GL_MULTISAMPLE_BUFFER_BIT0_QCOM 0x01000000
#define GL_MULTISAMPLE_BUFFER_BIT1_QCOM 0x02000000
#define GL_MULTISAMPLE_BUFFER_BIT2_QCOM 0x04000000
#define GL_MULTISAMPLE_BUFFER_BIT3_QCOM 0x08000000
#define GL_MULTISAMPLE_BUFFER_BIT4_QCOM 0x10000000
#define GL_MULTISAMPLE_BUFFER_BIT5_QCOM 0x20000000
#define GL_MULTISAMPLE_BUFFER_BIT6_QCOM 0x40000000
#define GL_MULTISAMPLE_BUFFER_BIT7_QCOM 0x80000000
#endif // GL_QCOM_tiled_rendering

#if !defined(GL_QCOM_writeonly_rendering)
#define GL_WRITEONLY_RENDERING_QCOM 0x8823
#endif // GL_QCOM_writeonly_rendering

#if !defined(GL_VIV_shader_binary)
#define GL_SHADER_BINARY_VIV 0x8FC4
#endif // GL_VIV_shader_binary

#endif
