
#include "vk/framebuffer.h"
#include "vk/context.h"
#include "vk/texture.h"
#include "vk/sampler.h"
#include "data/framebuffer.h"
#include "meta/texture.h"

using namespace psl;
using namespace core::gfx;
using namespace core::resource;
using namespace core;


framebuffer::framebuffer(const UID& uid, cache& cache, handle<context> context, handle<data::framebuffer> data)
	: m_Context(context), m_Data(deep_copy, data) // , m_Data(copy(cache, data))
{
	m_Framebuffers.resize(m_Data->framebuffers());
	m_Textures.reserve(m_Framebuffers.size() * m_Data->attachments().size());
	size_t index = 0u;
	for(const auto& attach : m_Data->attachments())
	{
		if(!add(cache, attach.texture(), attach.vkDescription(), index, (attach.shared()) ? 1u : m_Framebuffers.size()))
			throw std::runtime_error("");
		index += m_Framebuffers.size();
	}

	m_Sampler = core::resource::create_shared<core::gfx::sampler>(cache, m_Data);
	// m_Sampler.load(); // todo: this should not be working

	// now we create the renderpass that describes the framebuffer
	std::vector<vk::AttachmentDescription> attachmentDescriptions;
	std::transform(std::begin(m_Bindings), std::end(m_Bindings), std::back_inserter(attachmentDescriptions),
				   [](const auto& binding) { return binding.description; });

	// Collect attachment references
	std::vector<vk::AttachmentReference> colorReferences;
	vk::AttachmentReference depthReference;
	bool hasDepth = false;
	bool hasColor = false;

	uint32_t attachmentIndex = 0;

	for(auto& binding : m_Bindings)
	{
		if(utility::vulkan::is_depthstencil(binding.description.format))
		{
			// Only one depth attachment allowed
			// assert(!hasDepth);
			depthReference.attachment = attachmentIndex;
			depthReference.layout	 = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			hasDepth				  = true;
		}
		else
		{
			colorReferences.push_back({attachmentIndex, vk::ImageLayout::eColorAttachmentOptimal});
			hasColor = true;
		}
		attachmentIndex++;
	};

	// Default render pass setup uses only one subpass
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	if(hasColor)
	{
		subpass.pColorAttachments	= colorReferences.data();
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
	}
	if(hasDepth)
	{
		subpass.pDepthStencilAttachment = &depthReference;
	}

	// Use subpass dependencies for attachment layout transitions
	std::array<vk::SubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass	= VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass	= 0;
	dependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
	dependencies[0].dstAccessMask =
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	dependencies[1].srcSubpass   = 0;
	dependencies[1].dstSubpass   = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	dependencies[1].srcAccessMask =
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
	dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

	// Create render pass
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.pAttachments	= attachmentDescriptions.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.subpassCount	= 1;
	renderPassInfo.pSubpasses	  = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies   = dependencies.data();
	utility::vulkan::check(m_Context->device().createRenderPass(&renderPassInfo, nullptr, &m_RenderPass));

	// pre-allocate attachmentviews, and fill in all the data in the FBCI
	// we will override the views in the upcoming loop, where the device also will create the FBOs
	std::vector<vk::ImageView> attachmentViews;
	attachmentViews.resize(m_Bindings.size());

	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.renderPass		= m_RenderPass;
	framebufferInfo.pAttachments	= attachmentViews.data();
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
	framebufferInfo.width			= m_Data->width();
	framebufferInfo.height			= m_Data->height();
	framebufferInfo.layers			= 0;

	for(auto i = 0u; i < m_Data->framebuffers(); ++i)
	{
		// Find. max number of layers across attachments && store all attachment views
		uint32_t maxLayers = 0u;

		for(const auto& binding : m_Bindings)
		{
			attachmentViews[binding.index] = binding.attachments[i].view;
			maxLayers					   = std::max(maxLayers, binding.attachments[i].subresourceRange.layerCount);
		}

		framebufferInfo.layers = maxLayers;
		utility::vulkan::check(m_Context->device().createFramebuffer(&framebufferInfo, nullptr, &m_Framebuffers[i]));
	}
}

framebuffer::~framebuffer()
{
	m_Context->device().destroyRenderPass(m_RenderPass);
	for(auto& fb : m_Framebuffers) m_Context->device().destroyFramebuffer(fb);
}

bool framebuffer::add(core::resource::cache& cache, const UID& uid, vk::AttachmentDescription description, size_t index,
					  size_t count)
{
	auto res = cache.library().get<core::meta::texture>(uid);
	if(!res) return false;

	auto meta = res.value();

	binding& binding		   = m_Bindings.emplace_back();
	binding.index			   = index;
	binding.description		   = description;
	binding.description.format = meta->format();
	for(auto i = index; i < index + count; ++i)
	{
		auto texture = core::resource::create<core::gfx::texture>(cache, uid);
		if(texture.resource_state() != core::resource::state::LOADED) texture.load(m_Context);
		m_Textures.push_back(texture);
		attachment& attachment		= binding.attachments.emplace_back();
		attachment.view				= texture->view();
		attachment.memory			= texture->memory();
		attachment.subresourceRange = texture->subResourceRange();
		attachment.image			= texture->image();
	}

	return true;
}


std::vector<framebuffer::attachment> framebuffer::attachments(uint32_t index) const noexcept
{
	if(index >= m_Bindings.size()) return {};

	std::vector<framebuffer::attachment> res;
	std::transform(std::begin(m_Bindings), std::end(m_Bindings), std::back_inserter(res), [index](const auto& binding) {
		return (binding.attachments.size() > 1) ? binding.attachments[index] : binding.attachments[0];
	});
	return res;
}

std::vector<framebuffer::attachment> framebuffer::color_attachments(uint32_t index) const noexcept
{
	if(index >= m_Bindings.size()) return {};

	std::vector<framebuffer::attachment> res;
	auto bindings{ m_Bindings };
	auto end = std::remove_if(std::begin(bindings), std::end(bindings), [](const framebuffer::binding& binding) { return utility::vulkan::is_depthstencil(binding.description.format); });
	std::transform(std::begin(bindings), end, std::back_inserter(res), [index](const auto& binding) {
		return (binding.attachments.size() > 1) ? binding.attachments[index] : binding.attachments[0];
	});

	return res;
}
core::resource::handle<core::gfx::sampler> framebuffer::sampler() const noexcept { return m_Sampler; }
core::resource::handle<core::data::framebuffer> framebuffer::data() const noexcept { return m_Data; }
vk::RenderPass framebuffer::render_pass() const noexcept { return m_RenderPass; }
const std::vector<vk::Framebuffer>& framebuffer::framebuffers() const noexcept { return m_Framebuffers; }
vk::DescriptorImageInfo framebuffer::descriptor() const noexcept { return m_Descriptor; }
