
#include "core/vk/context.hpp"
#include "core/gfx/limits.hpp"
#include "core/logging.hpp"
#include "core/os/surface.hpp"
#include "core/paradigm.hpp"
#include "core/resource/resource.hpp"
#include "core/vk/conversion.hpp"
#include "psl/meta.hpp"
#include "psl/ustream.hpp"
#include "psl/utility/cast.hpp"

#ifdef PE_PLATFORM_LINUX
	// https://bugzilla.redhat.com/show_bug.cgi?id=130601 not a bug my ass, it's like the windows min/max..
	#undef minor
	#undef major
#endif

#ifdef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
static vk::DynamicLoader dl;
#endif

using namespace psl;
using namespace core::ivk;
using namespace core::os;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCB(VkDebugReportFlagsEXT flags,
											 VkDebugReportObjectTypeEXT objectType,
											 uint64_t object,
											 size_t location,
											 int32_t messageCode,
											 const char* layer_prefix,
											 const char* msg,
											 void* pUserData) {
	psl::string message {msg};
	message.append(" [");
	message.append(layer_prefix);
	message.append("]");
	switch(flags) {
	case VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT:
		core::ivk::log->info(message.c_str());
		break;
	case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
		core::ivk::log->warn(message.c_str());
		break;
	case VK_DEBUG_REPORT_WARNING_BIT_EXT:
		core::ivk::log->warn(message.c_str());
		core::ivk::log->flush();
		break;
	case VK_DEBUG_REPORT_ERROR_BIT_EXT:
		core::ivk::log->error(message.c_str());
		core::ivk::log->flush();
		break;
	case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
		core::ivk::log->debug(message.c_str());
		break;
	case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
	default:
		core::ivk::log->info(message.c_str());
		break;
	}
	return false;	 // always return false
}

inline psl::string8_t size_denotation(vk::DeviceSize size) {
	static const std::vector<psl::string8::view> SUFFIXES {{"B", "KB", "MB", "GB", "TB", "PB"}};
	size_t suffixIndex = 0;
	while(suffixIndex < SUFFIXES.size() - 1 && size > 1024) {
		size >>= 10;
		++suffixIndex;
	}

	psl::string8::stream buffer;
	buffer << size << " " << SUFFIXES[suffixIndex];
	return buffer.str();
}

struct VKAPIVersion {
  public:
	explicit VKAPIVersion(uint32_t version)
		: major((uint32_t)(version) >> 22), minor(((uint32_t)(version) >> 12) & 0x3ff),
		  patch((uint32_t)(version)&0xfff) {};

	bool operator==(const VKAPIVersion& other) const {
		return (major == other.major && minor == other.minor && patch == other.patch);
	};
	bool operator!=(const VKAPIVersion& other) const {
		return (major != other.major || minor != other.minor || patch != other.patch);
	};
	bool operator>(const VKAPIVersion& other) const {
		return (major > other.major && minor > other.minor && patch > other.patch);
	};
	bool operator>=(const VKAPIVersion& other) const {
		return (major >= other.major && minor >= other.minor && patch >= other.patch);
	};
	bool operator<(const VKAPIVersion& other) const {
		return (major < other.major && minor < other.minor && patch < other.patch);
	};
	bool operator<=(const VKAPIVersion& other) const {
		return (major <= other.major && minor <= other.minor && patch <= other.patch);
	};
	const uint32_t major;
	const uint32_t minor;
	const uint32_t patch;
};
context::context(core::resource::cache_t& cache,
				 const core::resource::metadata& metaData,
				 psl::meta::file* metaFile,
				 psl::string8::view name,
				 uint32_t deviceIndex)
	: m_DeviceIndex(deviceIndex) {
#ifndef VK_STATIC
	// todo load library ourselves, not through the vulkan dynamic loader solution
	// HMODULE module = LoadLibraryA("vulkan-1.dll");
	// if (!module)
	//	throw std::runtime_error("no vulkan library detected");

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
	  dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

	uint32_t extensionCount = 0;
	auto extensions			= vk::enumerateInstanceExtensionProperties();
	core::ivk::log->info("instance extensions: {}", extensions.value.size());
	for(const auto& ext : extensions.value) {
		core::ivk::log->info("instance extension: {}", psl::string(&ext.extensionName[0]));
	}
	uint32_t instanceCount = 0;
	auto instances		   = vk::enumerateInstanceLayerProperties();

	core::ivk::log->info("instance layers: {}", instances.value.size());
	for(const auto& inst : instances.value) {
		core::ivk::log->info("instance layer: {}", psl::string(&inst.layerName[0]));
	}
	if(m_Validated) {
		core::ivk::log->info("Enabling 'VK_LAYER_KHRONOS_validation'");
		m_InstanceLayerList.push_back("VK_LAYER_KHRONOS_validation");
		m_InstanceExtensionList.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	m_DeviceExtensionList.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	m_InstanceExtensionList.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

	m_InstanceExtensionList.push_back(VK_SURFACE_EXTENSION_NAME);

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = name.data();
	appInfo.pEngineName		 = APPLICATION_NAME.data();
	psl_assert(VERSION_MAJOR < std::pow(2, 10), "VERSION_MAJOR {} was higher than {}", VERSION_MAJOR, std::pow(2, 10));
	psl_assert(VERSION_MINOR < std::pow(2, 10), "VERSION_MINOR {} was higher than {}", VERSION_MINOR, std::pow(2, 10));
	psl_assert(VERSION_PATCH < std::pow(2, 12), "VERSION_PATCH {} was higher than {}", VERSION_PATCH, std::pow(2, 12));
	appInfo.engineVersion = (((VERSION_MAJOR) << 22) | ((VERSION_MINOR) << 12) | (VERSION_PATCH));
	appInfo.apiVersion	  = VK_API_VERSION_LATEST;

	vk::InstanceCreateInfo instanceCI;
	instanceCI.pApplicationInfo = &appInfo;

	if(m_InstanceLayerList.size() > 0) {
		instanceCI.enabledLayerCount   = (uint32_t)m_InstanceLayerList.size();
		instanceCI.ppEnabledLayerNames = m_InstanceLayerList.data();
	}
	if(m_InstanceExtensionList.size() > 0) {
		instanceCI.enabledExtensionCount   = (uint32_t)m_InstanceExtensionList.size();
		instanceCI.ppEnabledExtensionNames = m_InstanceExtensionList.data();
	}

	if(!core::utility::vulkan::check(vk::createInstance(&instanceCI, nullptr, &m_Instance))) {
		core::ivk::log->critical("Could not create a Vulkan instance.");
		std::exit(-1);
	}
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);

	init_debug();
	init_device();

	VKAPIVersion apiversion(appInfo.apiVersion);
	VKAPIVersion gpuapiversion(m_PhysicalDeviceProperties.apiVersion);

	core::ivk::log->info("Vulkan Context Created");
	core::ivk::log->info("API Version:         {0}.{1}.{2}", apiversion.major, apiversion.minor, apiversion.patch);
	if(gpuapiversion != apiversion) {
		core::ivk::log->warn(
		  "API Version(Actual): {0}.{1}.{2}", gpuapiversion.major, gpuapiversion.minor, gpuapiversion.patch);
	}
	core::ivk::log->info("Driver Version: {}", m_PhysicalDeviceProperties.driverVersion);
	psl::string8_t vendor;
	switch(m_PhysicalDeviceProperties.vendorID) {
	case 0x1002:
		vendor = "AMD";
		break;
	case 0x1010:
		vendor = "ImgTec";
		break;
	case 0x10DE:
		vendor = "NVidia";
		break;
	case 0x13B5:
		vendor = "ARM";
		break;
	case 0x5143:
		vendor = "Qualcomm";
		break;
	case 0x8086:
		vendor = "Intel";
		break;
	default:
		vendor = "Unknown";
	}
	core::ivk::log->info("Vendor:         {}", vendor);
	core::ivk::log->info("Device Name:    {}", m_PhysicalDeviceProperties.deviceName);
	core::ivk::log->info("Device Type:    {}", vk::to_string(m_PhysicalDeviceProperties.deviceType));
	core::ivk::log->info("Device ID:      {}", std::to_string(m_PhysicalDeviceProperties.deviceID));
	core::ivk::log->info("Memory Heaps:   {}", m_PhysicalDeviceMemoryProperties.memoryHeapCount);

	for(size_t i = 0; i < m_PhysicalDeviceMemoryProperties.memoryHeapCount; ++i) {
		const auto& heap = m_PhysicalDeviceMemoryProperties.memoryHeaps[i];
		core::ivk::log->info("\tHeap {0} flags {1} size {2}", i, vk::to_string(heap.flags), size_denotation(heap.size));
		if(heap.flags == vk::MemoryHeapFlagBits::eDeviceLocal) {
			m_DeviceMemory.availableMemory += heap.size;
		} else {
			m_HostMemory.availableMemory += heap.size;
		}
	}

	core::ivk::log->info("Memory Types:  {}", m_PhysicalDeviceMemoryProperties.memoryTypeCount);
	for(size_t i = 0; i < m_PhysicalDeviceMemoryProperties.memoryTypeCount; ++i) {
		const auto type = m_PhysicalDeviceMemoryProperties.memoryTypes[i];
		core::ivk::log->info("\tType {0} flags {1} heap {2}", i, vk::to_string(type.propertyFlags), type.heapIndex);
	}

	core::ivk::log->info("Queues:");
	std::vector<vk::QueueFamilyProperties> queueProps = m_PhysicalDevice.getQueueFamilyProperties();

	for(size_t i = 0; i < queueProps.size(); ++i) {
		const auto& queueFamilyProperties = queueProps[i];
		core::ivk::log->info("Queue Family: {}", i);
		core::ivk::log->info("\tQueue Family Flags: {}", vk::to_string(queueFamilyProperties.queueFlags));
		core::ivk::log->info("\tQueue Count: {}", queueFamilyProperties.queueCount);
	}

	init_command_pool();
	init_descriptor_pool();

	{
		psl::string information;

		core::ivk::log->info(
		  "\ndevice limits\n"
		  "	textures\n"
		  "		dimension 1D 	{0}\n"
		  "		dimension 2D 	{1}\n"
		  "		dimension 3D 	{2}\n"
		  "		dimension cube 	{3}\n"
		  "		array layers 	{4}\n"
		  "\n"
		  "	buffer\n"
		  "		max texel buffer element 			{5}\n"
		  "		max uniform buffer range 			{6}\n"
		  "		max storage buffer range 			{7}\n"
		  "		memory allocations 					{8}\n"
		  "		sampler allocations 				{9}\n"
		  "		buffer image granularity 			{10}\n"
		  "		sparse address space size			{11}\n"
		  "		host min memory map alignment		{12}\n"
		  "		min texel buffer offset alignment	{13}\n"
		  "		min uniform buffer offset alignment	{14}\n"
		  "		min storage buffer offset alginment	{15}\n"
		  "		min/max texel offset				{16} - {17}\n"
		  "		min/max texel gather offset			{18} - {19}\n"
		  "		min/max interpolation offset		{20} - {21}\n"
		  "		sub pixel interpolation offset bits	{22}\n"
		  "\n"
		  "	descriptor set\n"
		  "		samplers 					{23}\n"
		  "		uniform buffers 			{24}\n"
		  "		uniform buffers (dynamic) 	{25}\n"
		  "		storage buffers				{26}\n"
		  "		storage buffers (dynamic)	{27}\n"
		  "		sampled images				{28}\n"
		  "		storage images				{29}\n"
		  "		input attachments			{30}\n"
		  "\n"
		  "	shader statistics\n"
		  "		push constant size						{31}\n"
		  "		max bound descriptor sets				{32}\n"
		  "		per stage descriptor samplers			{33}\n"
		  "		per stage descriptor storage buffers	{113}\n"
		  "		per stage uniform buffers				{34}\n"
		  "		per stage sampled images				{35}\n"
		  "		per stage storage images				{36}\n"
		  "		per stage input attachments				{37}\n"
		  "		per stage resources						{38}\n"
		  "		sub texel precision bits				{39}\n"
		  "		mipmap precision bits					{40}\n"
		  "		sampler lod bias						{41}\n"
		  "		sampler anisotropy						{42}\n"
		  "\n"
		  "		vertex stage\n"
		  "			input attributes		{43}\n"
		  "			input bindings			{44}\n"
		  "			input attribute offset 	{45}\n"
		  "			input binding stride 	{46}\n"
		  "			output components		{47}\n"
		  "\n"
		  "		tesselation stage\n"
		  "			generation level	{48}\n"
		  "			patch size			{49}\n"
		  "			control per vertex input components	{50}\n"
		  "			control per vertex output components	{51}\n"
		  "			control per patch output components		{52}\n"
		  "			control total output components			{53}\n"
		  "			evaluation input components				{54}\n"
		  "			evaluation output components			{55}\n"
		  "\n"
		  "		geometry stage\n"
		  "			shader invocations		{56}\n"
		  "			input components		{57}\n"
		  "			output components		{58}\n"
		  "			output vertices			{59}\n"
		  "			total output components	{60}\n"
		  "\n"
		  "		fragment stage\n"
		  "			input components 			{61}\n"
		  "			output attachments			{62}\n"
		  "			dual source attachments		{63}\n"
		  "			combined output resources	{64}\n"
		  "\n"
		  "		compute stage\n"
		  "			shared memory size					{65}\n"
		  "			work group count [0] - [1] - [2]	{66} - {67} - {68}\n"
		  "			work group invocations				{69}\n"
		  "			work group size  [0] - [1] - [2]	{70} - {71} - {72}\n"
		  "\n"
		  "	viewports\n"
		  "		count					{73}\n"
		  "		dimensions 	 [0] - [1]	{74} - {75}\n"
		  "		bounds range [0] - [1]	{76} - {77}\n"
		  "		sub pixel bits			{78}\n"
		  "\n"
		  "	framebuffer\n"
		  "		width 					{79}\n"
		  "		height 					{80}\n"
		  "		layers 					{81}\n"
		  "		color sample counts		{82}\n"
		  "		depth sample counts		{83}\n"
		  "		stencil sample counts	{84}\n"
		  "		no attachments sample counts	{85}\n"
		  "		color attachments				{86}\n"
		  "		sub pixel precision bits		{87}\n"
		  "\n"
		  "	sampled images (VK_IMAGE_USAGE_SAMPLED_BITS)\n"
		  "		color sample counts		{88}\n"
		  "		integer sample counts 	{89}\n"
		  "		depth sample counts		{90}\n"
		  "		stencil sample counts	{91}\n"
		  "\n"
		  "	sampled images (VK_IMAGE_USAGE_STORAGE_BIT)\n"
		  "		sample counts	{92}\n"
		  "\n"
		  "	misc\n"
		  "		draw indexed index value	{93}\n"
		  "		draw indirect count			{94}\n"
		  "		sample maskwords (MSAA)		{95}\n"
		  "\n"
		  "		timestamp compute and graphics\n"
		  "			enabled					{96}\n"
		  "			timestamp period		{97}\n"
		  "\n"
		  "		clip distances						{98}\n"
		  "		cull distances						{99}\n"
		  "		combined clip and cull distances	{100}\n"
		  "		discrete queue priorities			{101}\n"
		  "\n"
		  "		point size\n"
		  "			range [0] - [1]		{102} - {103}\n"
		  "			granularity			{104}\n"
		  "\n"
		  "		line width\n"
		  "			range [0] - [1]		{105} - {106}\n"
		  "			granularity			{107}\n"
		  "			strict lines		{108}\n"
		  "\n"
		  "		standard sample locations 				{109}\n"
		  "		optimal buffer copy offset alignment 	{110}\n"
		  "		optimal buffer copy row pitch alignment	{111}\n"
		  "		non coherent atom size					{112}",
		  // texture
		  m_PhysicalDeviceProperties.limits.maxImageDimension1D,
		  m_PhysicalDeviceProperties.limits.maxImageDimension2D,
		  m_PhysicalDeviceProperties.limits.maxImageDimension3D,
		  m_PhysicalDeviceProperties.limits.maxImageDimensionCube,
		  m_PhysicalDeviceProperties.limits.maxImageArrayLayers,
		  // buffer
		  m_PhysicalDeviceProperties.limits.maxTexelBufferElements,
		  m_PhysicalDeviceProperties.limits.maxUniformBufferRange,
		  m_PhysicalDeviceProperties.limits.maxStorageBufferRange,
		  m_PhysicalDeviceProperties.limits.maxMemoryAllocationCount,
		  m_PhysicalDeviceProperties.limits.maxSamplerAllocationCount,
		  m_PhysicalDeviceProperties.limits.bufferImageGranularity,
		  m_PhysicalDeviceProperties.limits.sparseAddressSpaceSize,
		  m_PhysicalDeviceProperties.limits.minMemoryMapAlignment,
		  m_PhysicalDeviceProperties.limits.minTexelBufferOffsetAlignment,
		  m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
		  m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment,
		  m_PhysicalDeviceProperties.limits.minTexelOffset,
		  m_PhysicalDeviceProperties.limits.maxTexelOffset,
		  m_PhysicalDeviceProperties.limits.minTexelGatherOffset,
		  m_PhysicalDeviceProperties.limits.maxTexelGatherOffset,
		  m_PhysicalDeviceProperties.limits.minInterpolationOffset,
		  m_PhysicalDeviceProperties.limits.maxInterpolationOffset,
		  m_PhysicalDeviceProperties.limits.subPixelInterpolationOffsetBits,
		  // descriptor set
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetSamplers,
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffers,
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic,
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetStorageBuffers,
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic,
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetSampledImages,
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetStorageImages,
		  m_PhysicalDeviceProperties.limits.maxDescriptorSetInputAttachments,
		  // shader
		  m_PhysicalDeviceProperties.limits.maxPushConstantsSize,
		  m_PhysicalDeviceProperties.limits.maxBoundDescriptorSets,
		  m_PhysicalDeviceProperties.limits.maxPerStageDescriptorSamplers,
		  m_PhysicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers,
		  m_PhysicalDeviceProperties.limits.maxPerStageDescriptorSampledImages,
		  m_PhysicalDeviceProperties.limits.maxPerStageDescriptorStorageImages,
		  m_PhysicalDeviceProperties.limits.maxPerStageDescriptorInputAttachments,
		  m_PhysicalDeviceProperties.limits.maxPerStageResources,
		  m_PhysicalDeviceProperties.limits.subTexelPrecisionBits,
		  m_PhysicalDeviceProperties.limits.mipmapPrecisionBits,
		  m_PhysicalDeviceProperties.limits.maxSamplerLodBias,
		  m_PhysicalDeviceProperties.limits.maxSamplerAnisotropy,
		  // shader - vertex stage
		  m_PhysicalDeviceProperties.limits.maxVertexInputAttributes,
		  m_PhysicalDeviceProperties.limits.maxVertexInputBindings,
		  m_PhysicalDeviceProperties.limits.maxVertexInputAttributeOffset,
		  m_PhysicalDeviceProperties.limits.maxVertexInputBindingStride,
		  m_PhysicalDeviceProperties.limits.maxVertexOutputComponents,
		  // shader - tesselation stage
		  m_PhysicalDeviceProperties.limits.maxTessellationGenerationLevel,
		  m_PhysicalDeviceProperties.limits.maxTessellationPatchSize,
		  m_PhysicalDeviceProperties.limits.maxTessellationControlPerVertexInputComponents,
		  m_PhysicalDeviceProperties.limits.maxTessellationControlPerVertexOutputComponents,
		  m_PhysicalDeviceProperties.limits.maxTessellationControlPerPatchOutputComponents,
		  m_PhysicalDeviceProperties.limits.maxTessellationControlTotalOutputComponents,
		  m_PhysicalDeviceProperties.limits.maxTessellationEvaluationInputComponents,
		  m_PhysicalDeviceProperties.limits.maxTessellationEvaluationOutputComponents,
		  // shader - geometry stage
		  m_PhysicalDeviceProperties.limits.maxGeometryShaderInvocations,
		  m_PhysicalDeviceProperties.limits.maxGeometryInputComponents,
		  m_PhysicalDeviceProperties.limits.maxGeometryOutputComponents,
		  m_PhysicalDeviceProperties.limits.maxGeometryOutputVertices,
		  m_PhysicalDeviceProperties.limits.maxGeometryTotalOutputComponents,
		  // shader - fragment stage
		  m_PhysicalDeviceProperties.limits.maxFragmentInputComponents,
		  m_PhysicalDeviceProperties.limits.maxFragmentOutputAttachments,
		  m_PhysicalDeviceProperties.limits.maxFragmentDualSrcAttachments,
		  m_PhysicalDeviceProperties.limits.maxFragmentCombinedOutputResources,
		  // shader - compute stage
		  m_PhysicalDeviceProperties.limits.maxComputeSharedMemorySize,
		  m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0],
		  m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[1],
		  m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[2],
		  m_PhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations,
		  m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0],
		  m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[1],
		  m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[2],
		  // viewports
		  m_PhysicalDeviceProperties.limits.maxViewports,
		  m_PhysicalDeviceProperties.limits.maxViewportDimensions[0],
		  m_PhysicalDeviceProperties.limits.maxViewportDimensions[1],
		  m_PhysicalDeviceProperties.limits.viewportBoundsRange[0],
		  m_PhysicalDeviceProperties.limits.viewportBoundsRange[1],
		  m_PhysicalDeviceProperties.limits.viewportSubPixelBits,
		  // framebuffer
		  m_PhysicalDeviceProperties.limits.maxFramebufferWidth,
		  m_PhysicalDeviceProperties.limits.maxFramebufferHeight,
		  m_PhysicalDeviceProperties.limits.maxFramebufferLayers,
		  m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts.operator unsigned int(),
		  m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts.operator unsigned int(),
		  m_PhysicalDeviceProperties.limits.framebufferStencilSampleCounts.operator unsigned int(),
		  m_PhysicalDeviceProperties.limits.framebufferNoAttachmentsSampleCounts.operator unsigned int(),
		  m_PhysicalDeviceProperties.limits.maxColorAttachments,
		  m_PhysicalDeviceProperties.limits.subPixelPrecisionBits,
		  // sampled images - usage sampled bit
		  m_PhysicalDeviceProperties.limits.sampledImageColorSampleCounts.operator unsigned int(),
		  m_PhysicalDeviceProperties.limits.sampledImageIntegerSampleCounts.operator unsigned int(),
		  m_PhysicalDeviceProperties.limits.sampledImageDepthSampleCounts.operator unsigned int(),
		  m_PhysicalDeviceProperties.limits.sampledImageStencilSampleCounts.operator unsigned int(),
		  // sampled images - storage sampled bit
		  m_PhysicalDeviceProperties.limits.storageImageSampleCounts.operator unsigned int(),
		  // misc
		  m_PhysicalDeviceProperties.limits.maxDrawIndexedIndexValue,
		  m_PhysicalDeviceProperties.limits.maxDrawIndirectCount,
		  m_PhysicalDeviceProperties.limits.maxSampleMaskWords,
		  // timestamp
		  (m_PhysicalDeviceProperties.limits.timestampComputeAndGraphics) ? true : false,
		  m_PhysicalDeviceProperties.limits.timestampPeriod,
		  m_PhysicalDeviceProperties.limits.maxClipDistances,
		  m_PhysicalDeviceProperties.limits.maxCullDistances,
		  m_PhysicalDeviceProperties.limits.maxCombinedClipAndCullDistances,
		  m_PhysicalDeviceProperties.limits.discreteQueuePriorities,
		  // point size
		  m_PhysicalDeviceProperties.limits.pointSizeRange[0],
		  m_PhysicalDeviceProperties.limits.pointSizeRange[1],
		  m_PhysicalDeviceProperties.limits.pointSizeGranularity,
		  // line width
		  m_PhysicalDeviceProperties.limits.lineWidthRange[0],
		  m_PhysicalDeviceProperties.limits.lineWidthRange[1],
		  m_PhysicalDeviceProperties.limits.lineWidthGranularity,
		  (m_PhysicalDeviceProperties.limits.strictLines) ? true : false,

		  (m_PhysicalDeviceProperties.limits.standardSampleLocations) ? true : false,
		  m_PhysicalDeviceProperties.limits.optimalBufferCopyOffsetAlignment,
		  m_PhysicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment,
		  m_PhysicalDeviceProperties.limits.nonCoherentAtomSize,

		  // forgot this one
		  m_PhysicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers);
	}

	m_Limits.uniform.alignment =
	  psl::narrow_cast<size_t>(m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
	m_Limits.uniform.size = psl::narrow_cast<size_t>(m_PhysicalDeviceProperties.limits.maxUniformBufferRange);
	m_Limits.storage.alignment =
	  psl::narrow_cast<size_t>(m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
	m_Limits.storage.size		 = psl::narrow_cast<size_t>(m_PhysicalDeviceProperties.limits.maxStorageBufferRange);
	m_Limits.memorymap.alignment = psl::narrow_cast<size_t>(m_PhysicalDeviceProperties.limits.minMemoryMapAlignment);
	m_Limits.memorymap.size		 = psl::narrow_cast<size_t>(std::numeric_limits<uint64_t>::max());

	vk::Format format;
	if(core::utility::vulkan::supported_depthformat(m_PhysicalDevice, &format))
		m_Limits.supported_depthformat = core::gfx::conversion::to_format(format);
	else
		m_Limits.supported_depthformat = core::gfx::format_t::undefined;

	m_Limits.compute.workgroup.count[0] = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0];
	m_Limits.compute.workgroup.count[1] = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[1];
	m_Limits.compute.workgroup.count[2] = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupCount[2];

	m_Limits.compute.workgroup.size[0] = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0];
	m_Limits.compute.workgroup.size[1] = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[1];
	m_Limits.compute.workgroup.size[2] = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupSize[2];

	m_Limits.compute.workgroup.invocations = m_PhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations;
}

context::~context() {
	m_Device.waitIdle();
	deinit_descriptor_pool();
	deinit_command_pool();
	deinit_device();
	deinit_debug();
	m_Instance.destroy();

	core::ivk::log->info("vulkan context destroyed");
}

void context::init_debug() {
	if(m_Validated) {
		vk::DebugReportCallbackCreateInfoEXT callbackCreateInfo {};
		callbackCreateInfo.flags = vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::eWarning |
								   vk::DebugReportFlagBitsEXT::ePerformanceWarning |
								   vk::DebugReportFlagBitsEXT ::eError | vk::DebugReportFlagBitsEXT::eDebug;

		callbackCreateInfo.pfnCallback = &VulkanDebugCB;

		vk::Result success;
		std::tie(success, m_DebugReport) = m_Instance.createDebugReportCallbackEXT(callbackCreateInfo);
		core::utility::vulkan::check(success);
	}
}

void context::deinit_debug() {
	if(m_Validated) {
		m_Instance.destroyDebugReportCallbackEXT(m_DebugReport);
	}
}

bool context::queue_index(vk::QueueFlags flag, vk::Queue& queue, uint32_t& queueIndex) {
	// Find a queue that supports graphics operations
	std::vector<vk::QueueFamilyProperties> queueProps = m_PhysicalDevice.getQueueFamilyProperties();
	std::optional<uint32_t> secondBest				  = std::nullopt;
	for(queueIndex = 0; queueIndex < queueProps.size(); queueIndex++) {
		if(queueProps[queueIndex].queueFlags & flag)
			secondBest = queueIndex;

		if(queueProps[queueIndex].queueFlags == flag)
			break;
	}
	if(queueIndex >= queueProps.size() && secondBest.has_value())
		queueIndex = secondBest.value();
	if(queueIndex >= queueProps.size()) {
		core::ivk::log->warn("Could not find the appropriate queue enabled bit for {}", vk::to_string(flag));
		return false;
	}
	return true;
}

bool context::consume(vk::MemoryHeapFlagBits type, vk::DeviceSize amount) {
	if(type == vk::MemoryHeapFlagBits::eDeviceLocal) {
		if(amount > m_DeviceMemory.availableMemory)
			return false;
		m_DeviceMemory.availableMemory -= amount;
	} else {
		if(amount > m_HostMemory.availableMemory)
			return false;
		m_HostMemory.availableMemory -= amount;
	}
	return true;
}

bool context::release(vk::MemoryHeapFlagBits type, vk::DeviceSize amount) {
	if(type == vk::MemoryHeapFlagBits::eDeviceLocal) {
		if(m_DeviceMemory.availableMemory + amount > m_DeviceMemory.maxMemory)
			return false;
		m_DeviceMemory.availableMemory += amount;
	} else {
		if(m_HostMemory.availableMemory + amount > m_HostMemory.maxMemory)
			return false;
		m_HostMemory.availableMemory += amount;
	}
	return true;
}

vk::DeviceSize context::remaining(vk::MemoryHeapFlagBits type) {
	return (type == vk::MemoryHeapFlagBits::eDeviceLocal) ? m_DeviceMemory.availableMemory
														  : m_HostMemory.availableMemory;
}

void context::init_device() {
	select_physical_device(m_Instance.enumeratePhysicalDevices().value);

	std::vector<vk::ExtensionProperties> extensions {m_PhysicalDevice.enumerateDeviceExtensionProperties().value};

	for(const auto& ext : extensions) {
		core::ivk::log->info("device extension: {}", psl::string(&ext.extensionName[0]));
	}

	std::vector<vk::LayerProperties> layers {m_PhysicalDevice.enumerateDeviceLayerProperties().value};

	for(const auto& lyr : layers) {
		core::ivk::log->info("device layer: {}", psl::string(&lyr.layerName[0]));
	}


	m_PhysicalDeviceProperties = m_PhysicalDevice.getProperties();

	m_PhysicalDeviceFeatures		 = m_PhysicalDevice.getFeatures();
	m_PhysicalDeviceMemoryProperties = m_PhysicalDevice.getMemoryProperties();

	bool has_transfer = queue_index(vk::QueueFlagBits::eTransfer, m_TransferQueue, m_TransferQueueIndex);
	if(!queue_index(vk::QueueFlagBits::eGraphics, m_Queue, m_GraphicsQueueIndex)) {
		core::ivk::log->critical("could not find a graphics queue, even though it was requested, crashing now...");
		exit(1);
	}

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
	if(m_TransferQueueIndex == m_GraphicsQueueIndex && has_transfer) {
		// Vulkan device
		std::array<float, 1> queuePriorities = {1.0f};
		queueCreateInfo.resize(1);
		queueCreateInfo[0].queueFamilyIndex = m_GraphicsQueueIndex;
		queueCreateInfo[0].queueCount		= 1;
		queueCreateInfo[0].pQueuePriorities = queuePriorities.data();

		if(!core::utility::vulkan::check(
			 create_device(queueCreateInfo.data(), (uint32_t)queueCreateInfo.size(), m_Device))) {
			core::ivk::log->critical("Could not create a Vulkan device.");
			std::exit(-1);
		}

		m_Device.getQueue(m_GraphicsQueueIndex, 0, &m_Queue);
		m_TransferQueue = m_Queue;
	} else {
		// Vulkan device
		std::array<float, 1> queuePriorities = {1.0f};
		queueCreateInfo.resize(2);
		queueCreateInfo[0].queueFamilyIndex = m_GraphicsQueueIndex;
		queueCreateInfo[0].queueCount		= 1;
		queueCreateInfo[0].pQueuePriorities = queuePriorities.data();

		queueCreateInfo[1].queueFamilyIndex = m_TransferQueueIndex;
		queueCreateInfo[1].queueCount		= 1;
		queueCreateInfo[1].pQueuePriorities = queuePriorities.data();


		if(!core::utility::vulkan::check(
			 create_device(queueCreateInfo.data(), (uint32_t)queueCreateInfo.size(), m_Device))) {
			core::ivk::log->critical("Could not create a Vulkan device.");
			std::exit(-1);
		}

		m_Device.getQueue(m_GraphicsQueueIndex, 0, &m_Queue);
		m_Device.getQueue(m_TransferQueueIndex, 0, &m_TransferQueue);
	}

#if !defined(VK_STATIC)
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device);
#endif
}

void context::deinit_device() {
	m_Device.destroy();
}


vk::Result context::create_device(vk::DeviceQueueCreateInfo* requestedQueues, uint32_t queueSize, vk::Device& device) {
	vk::DeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.pNext				  = NULL;
	deviceCreateInfo.queueCreateInfoCount = queueSize;
	deviceCreateInfo.pQueueCreateInfos	  = requestedQueues;
	core::log->info("{} is supported", m_PhysicalDeviceFeatures.samplerAnisotropy);
	core::log->flush();
	deviceCreateInfo.pEnabledFeatures = &m_PhysicalDeviceFeatures;

	deviceCreateInfo.enabledExtensionCount	 = (uint32_t)m_DeviceExtensionList.size();
	deviceCreateInfo.ppEnabledExtensionNames = m_DeviceExtensionList.data();

	if(m_Validated) {
		deviceCreateInfo.enabledLayerCount	 = (uint32_t)m_DeviceLayerList.size();
		deviceCreateInfo.ppEnabledLayerNames = m_DeviceLayerList.data();
	}

	return m_PhysicalDevice.createDevice(&deviceCreateInfo, nullptr, &device);
}


void context::init_command_pool() {
	vk::CommandPoolCreateInfo CI;
	CI.queueFamilyIndex = m_GraphicsQueueIndex;
	CI.flags			= vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	core::utility::vulkan::check(m_Device.createCommandPool(&CI, nullptr, &m_CommandPool));


	CI.queueFamilyIndex = m_TransferQueueIndex;
	CI.flags			= vk::CommandPoolCreateFlagBits::eTransient;
	core::utility::vulkan::check(m_Device.createCommandPool(&CI, nullptr, &m_TransferCommandPool));
}

void context::deinit_command_pool() {
	m_Device.destroyCommandPool(m_TransferCommandPool);
	m_Device.destroyCommandPool(m_CommandPool);
	m_CommandPool		  = nullptr;
	m_TransferCommandPool = nullptr;
}

void context::init_descriptor_pool() {
	// We need to tell the API the number of max. requested descriptors per type
	std::vector<vk::DescriptorPoolSize> typeCounts = {
	  core::utility::vulkan::defaults::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64),
	  core::utility::vulkan::defaults::descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16),
	  core::utility::vulkan::defaults::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64),
	  core::utility::vulkan::defaults::descriptor_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 64),
	  core::utility::vulkan::defaults::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64)};
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// typeCounts[1].descriptorCount = 2;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	vk::DescriptorPoolCreateInfo descriptorPoolInfo =
	  core::utility::vulkan::defaults::descriptor_pool_ci((uint32_t)typeCounts.size(), typeCounts.data(), 32);
	descriptorPoolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	core::utility::vulkan::check(m_Device.createDescriptorPool(&descriptorPoolInfo, nullptr, &m_DescriptorPool));
}

void context::deinit_descriptor_pool() {
	m_Device.destroyDescriptorPool(m_DescriptorPool);
}

vk::Bool32
context::memory_type(uint32_t typeBits, const vk::MemoryPropertyFlags& properties, uint32_t* typeIndex) const noexcept {
	for(uint32_t i = 0; i < 32; i++) {
		if((typeBits & 1) == 1) {
			if((m_PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}

uint32_t context::memory_type(uint32_t typeBits, const vk::MemoryPropertyFlags& properties) const {
	for(uint32_t i = 0; i < 32; i++) {
		if((typeBits & 1) == 1) {
			if((m_PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		typeBits >>= 1;
	}

	throw std::runtime_error("could not find memory type");
}

void context::flush(vk::CommandBuffer commandBuffer, bool free) {
	if(!commandBuffer) {
		return;
	}

	core::utility::vulkan::check(commandBuffer.end());

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &commandBuffer;

	vk::FenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.flags				= vk::FenceCreateFlagBits(0u);
	// fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	// fenceCreateInfo.flags = 0u;
	auto [result, fence] = m_Device.createFence(fenceCreateInfo, nullptr);
	core::utility::vulkan::check(result);

	core::utility::vulkan::check(m_Queue.submit(1, &submitInfo, fence));
	core::utility::vulkan::check(m_Device.waitForFences(1, &fence, VK_TRUE, 100000000000 /*nanoseconds*/));
	m_Device.destroyFence(fence);

	if(free) {
		m_Device.freeCommandBuffers(m_CommandPool, 1, &commandBuffer);
	}
}

bool has_queue(const vk::PhysicalDevice& device, vk::QueueFlags flag) noexcept {
	std::vector<vk::QueueFamilyProperties> queueProps = device.getQueueFamilyProperties();
	return std::any_of(std::begin(queueProps), std::end(queueProps), [flag](const auto& queue) noexcept {
		return queue.queueFlags & flag;
	});
}

bool is_valid_device(const vk::PhysicalDevice& device) noexcept {
	// auto properties = device.getProperties();

	auto features = device.getFeatures();
	// auto memory_properties = device.getMemoryProperties();
	return features.samplerAnisotropy && has_queue(device, vk::QueueFlagBits::eGraphics);
}


void context::select_physical_device(const std::vector<vk::PhysicalDevice>& allDevices) {
	if(m_DeviceIndex == std::numeric_limits<uint32_t>::max()) {
		for(auto i = 0u; i < allDevices.size(); ++i) {
			if(is_valid_device(allDevices[i])) {
				m_DeviceIndex = i;
				break;
			}
		}
	}

	if(m_DeviceIndex != std::numeric_limits<uint32_t>::max()) {
		m_PhysicalDevice = allDevices[m_DeviceIndex];
	} else {
		throw std::runtime_error("No valid physical device found that was suitable");
	}
}

const vk::Instance& context::instance() const noexcept {
	return m_Instance;
}

const vk::Device& context::device() const noexcept {
	return m_Device;
}

const vk::PhysicalDevice& context::physical_device() const noexcept {
	return m_PhysicalDevice;
}

const vk::PhysicalDeviceProperties& context::properties() const noexcept {
	return m_PhysicalDeviceProperties;
}

const vk::PhysicalDeviceFeatures& context::features() const noexcept {
	return m_PhysicalDeviceFeatures;
}

const vk::PhysicalDeviceMemoryProperties& context::memory_properties() const noexcept {
	return m_PhysicalDeviceMemoryProperties;
}

const vk::CommandPool& context::command_pool() const noexcept {
	return m_CommandPool;
}

const vk::CommandPool& context::transfer_command_pool() const noexcept {
	return m_TransferCommandPool;
}

const vk::DescriptorPool& context::descriptor_pool() const noexcept {
	return m_DescriptorPool;
}

const vk::Queue& context::queue() const noexcept {
	return m_Queue;
}

const vk::Queue& context::transfer_queue() const noexcept {
	return m_TransferQueue;
}

uint32_t context::graphics_queue_index() const noexcept {
	return m_GraphicsQueueIndex;
}

uint32_t context::transfer_que_index() const noexcept {
	return m_TransferQueueIndex;
}

bool context::acquireNextImage2KHR(optional_ref<uint64_t> out_version) const noexcept {
	return false;
}
