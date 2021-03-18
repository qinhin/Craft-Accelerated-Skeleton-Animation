#pragma once
#include "Global.h"


class Pipeline
{
public:
	VkDescriptorSetLayout mDescriptorSetLayouts[2];
	VkPipelineLayout mPipelineLayout;
	VkPipeline mPipeline;
	VkRenderPass mRenderPass;
	using BufferMemory = vulkan::BufferMemory;
	using ImageTexture = vulkan::ImageTexture;




	void init()
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();

		VkDescriptorSetLayoutBinding bindings0[1] = {
			vulkan::vkt::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr),
		};
		VkDescriptorSetLayoutBinding bindings1[2] = {
			vulkan::vkt::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr),
			vulkan::vkt::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr),
		};
		mDescriptorSetLayouts[0] = pLogicalDevice->createDescriptorSetLayout(bindings0, 1);
		mDescriptorSetLayouts[1] = pLogicalDevice->createDescriptorSetLayout(bindings1, 2);

		auto push_constant = vulkan::vkt::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(int));
		mPipelineLayout = pLogicalDevice->createPipelineLayout(mDescriptorSetLayouts, 2, &push_constant, 1);


		std::vector<VkAttachmentDescription> color_attachments;
		color_attachments =
		{
			vulkan::vkt::attachmentDescription(
				0,
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			),
			vulkan::vkt::attachmentDescription(
				0,
				VK_FORMAT_D32_SFLOAT,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			)
		};
		std::vector<VkAttachmentReference> color_references;
		color_references.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		VkAttachmentReference depth_reference = {};
		depth_reference.attachment = 1;
		depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = vulkan::vkt::subpassDescription(
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr,
			color_references.size(), color_references.data(),
			nullptr, // MSAA
			&depth_reference,
			0, nullptr
		);
		VkSubpassDependency dependency = vulkan::vkt::subpassDependency(
			VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			0
		);
		mRenderPass = pLogicalDevice->createRenderPass(0, color_attachments.size(), color_attachments.data(), 1, &subpass, 1, &dependency);

		VkPipelineInputAssemblyStateCreateInfo input_assembly = vulkan::vkt::pipelineInputAssemblyStateCreateInfo(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
		auto rasterization_state = vulkan::vkt::pipelineRasterizationStateCreateInfo(false, false, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, false, 0.0f, 0.0f, 0.0f, 1.0f);
		auto depth_stencil_state = vulkan::vkt::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

		auto color_blend_attachment = vulkan::vkt::pipelineColorBlendAttachmentState(false,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
		float blend_constants[4] = { 1.0f,1.0f,1.0f,1.0f };
		auto color_blend_state = vulkan::vkt::pipelineColorBlendStateCreateInfo(false, VK_LOGIC_OP_COPY, 1, &color_blend_attachment, blend_constants);
		VkViewport viewport = vulkan::vkt::viewport(0, 0, 1920, 1080, 0.0f, 1.0f);
		VkRect2D scissor = vulkan::vkt::rect2D(0, 0, 65535, 65535);
		VkPipelineViewportStateCreateInfo viewport_state = vulkan::vkt::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);
		VkPipelineMultisampleStateCreateInfo multisampling = vulkan::vkt::pipelineMultisampleStateCreateInfo(
			(VkSampleCountFlagBits)VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 0.0f, nullptr, VK_FALSE, VK_FALSE); // minSampleShading = 0.2f? VK_SAMPLE_COUNT_1_BIT / m_multiSampleCount

		std::array<VkDynamicState, 3> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
		VkPipelineDynamicStateCreateInfo dynamicState = vulkan::vkt::pipelineDynamicStateCreateInfo(
			dynamic_states.data(), dynamic_states.size(), 0);
		VkVertexInputBindingDescription binding_description = vulkan::vkt::vertexInputBindingDescription(0, sizeof(graphics::Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		std::array<VkVertexInputAttributeDescription, 7> attribute_descriptions = {
			vulkan::vkt::vertexInputAttributeDescription(0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(graphics::Vertex, mPosition)),
			vulkan::vkt::vertexInputAttributeDescription(1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(graphics::Vertex, mNormal)),
			vulkan::vkt::vertexInputAttributeDescription(2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(graphics::Vertex, mTexcoords[0])),
			vulkan::vkt::vertexInputAttributeDescription(3,0,VK_FORMAT_R32G32_SFLOAT,offsetof(graphics::Vertex, mTexcoords[1])),
			vulkan::vkt::vertexInputAttributeDescription(4,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(graphics::Vertex, mTangent)),
			vulkan::vkt::vertexInputAttributeDescription(5,0,VK_FORMAT_R32G32B32A32_UINT,offsetof(graphics::Vertex, mBoneIndices)),
			vulkan::vkt::vertexInputAttributeDescription(6,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(graphics::Vertex, mBoneWeights)),		
		};
		VkPipelineVertexInputStateCreateInfo vertex_input = vulkan::vkt::pipelineVertexInputStateCreateInfo(
			1, &binding_description, attribute_descriptions.size(), attribute_descriptions.data());

		auto shader_data = core::readFile("shaders/spv/debug_skinning.vert.spv");
		VkShaderModule vertShaderModule = pLogicalDevice->createShaderModule(shader_data.data(), shader_data.size());
		shader_data = core::readFile("shaders/spv/debug_skinning.frag.spv");
		VkShaderModule fragShaderModule = pLogicalDevice->createShaderModule(shader_data.data(), shader_data.size());
		VkPipelineShaderStageCreateInfo shader_stages[2] = {
			vulkan::vkt::pipelineShaderStageCreateInfo(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT),
			vulkan::vkt::pipelineShaderStageCreateInfo(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		VkGraphicsPipelineCreateInfo pipelineInfo = vulkan::vkt::graphicsPipelineCreateInfo(
			2, shader_stages, &vertex_input, &input_assembly, nullptr, &viewport_state, &rasterization_state, &multisampling,
			&depth_stencil_state, &color_blend_state, &dynamicState, mPipelineLayout, mRenderPass, 0, VK_NULL_HANDLE, 0);

		if (vkCreateGraphicsPipelines(pLogicalDevice->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		pLogicalDevice->destroyShaderModule(fragShaderModule);
		pLogicalDevice->destroyShaderModule(vertShaderModule);

	}


	void release()
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		pLogicalDevice->destroyPipeline(mPipeline);
		pLogicalDevice->destroyRenderPass(mRenderPass);
		pLogicalDevice->destroyPipelineLayout(mPipelineLayout);
		pLogicalDevice->destroyDescriptorSetLayout(mDescriptorSetLayouts[0]);
		pLogicalDevice->destroyDescriptorSetLayout(mDescriptorSetLayouts[1]);
	}


};