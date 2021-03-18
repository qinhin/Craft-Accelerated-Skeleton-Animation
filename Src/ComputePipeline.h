#pragma once
#include "Scene.h"

class ComputePipeline
{
public:

	using BufferMemory = vulkan::BufferMemory;

	VkPipeline mPipelines[2];
	VkDescriptorSetLayout mDescriptorSetLayout;
	VkPipelineLayout mPipelineLayout;
	VkDescriptorSet mDescriptorSet;
	vulkan::DescriptorPool mDescriptorPool;

	void init()
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		std::array<VkDescriptorSetLayoutBinding, 13> setLayoutBindings_inputAttachment =
		{
			vulkan::vkt::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(10, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(11, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
			vulkan::vkt::descriptorSetLayoutBinding(12, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,VK_SHADER_STAGE_COMPUTE_BIT),
		};
		this->mDescriptorSetLayout = pLogicalDevice->createDescriptorSetLayout(setLayoutBindings_inputAttachment.data(), setLayoutBindings_inputAttachment.size());
		this->mPipelineLayout = pLogicalDevice->createPipelineLayout(&mDescriptorSetLayout, 1, nullptr, 0);


		VkDescriptorPoolSize pool_sizes[2] = {
			vulkan::vkt::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 12),
			vulkan::vkt::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		};
		mDescriptorPool = pLogicalDevice->createDescriptorPool(pool_sizes, 2, 1);
		mDescriptorPool.create(&mDescriptorSetLayout, &mDescriptorSet, 1);


		VkShaderModule shaders[2];
		auto shader_data = core::readFile("./shaders/spv/animation_pass1.comp.spv");
		shaders[0] = pLogicalDevice->createShaderModule(shader_data.data(), shader_data.size());
		shader_data = core::readFile("./shaders/spv/animation_pass2.comp.spv");
		shaders[1] = pLogicalDevice->createShaderModule(shader_data.data(), shader_data.size());

		VkPipelineShaderStageCreateInfo compShaderStageInfos[2] = {};
		compShaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfos[0].stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfos[0].module = shaders[0];
		compShaderStageInfos[0].pName = "main";

		compShaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfos[1].stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfos[1].module = shaders[1];
		compShaderStageInfos[1].pName = "main";


		VkComputePipelineCreateInfo computePipelineCreateInfos[2]{};

		computePipelineCreateInfos[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfos[0].layout = mPipelineLayout;
		computePipelineCreateInfos[0].flags = 0;
		computePipelineCreateInfos[0].stage = compShaderStageInfos[0];
		computePipelineCreateInfos[1].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfos[1].layout = mPipelineLayout;
		computePipelineCreateInfos[1].flags = 0;
		computePipelineCreateInfos[1].stage = compShaderStageInfos[1];	

		if (vkCreateComputePipelines(pLogicalDevice->getLogicalDevice(), VK_NULL_HANDLE, 2, computePipelineCreateInfos, nullptr, mPipelines) != VK_SUCCESS)
			throw std::runtime_error("failed to create graphics pipeline!");


		pLogicalDevice->destroyShaderModule(shaders[0]);
		pLogicalDevice->destroyShaderModule(shaders[1]);

	}

	void release()
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		pLogicalDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
		pLogicalDevice->destroyPipelineLayout(mPipelineLayout);
		pLogicalDevice->destroy(mDescriptorPool);
		pLogicalDevice->destroyPipeline(mPipelines[0]);
		pLogicalDevice->destroyPipeline(mPipelines[1]);
	}

	void compute(BufferMemory buffers[13], int size)
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		VkDescriptorBufferInfo bufferInfos[13];
		VkWriteDescriptorSet bufferWrites[13];
		for (int i = 0; i < 13; i++)
		{
			bufferInfos[i] = vulkan::vkt::descriptorBufferInfo(buffers[i].mBuffer, 0, buffers[i].mSize);
			bufferWrites[i] = vulkan::vkt::writeDescriptorSet(mDescriptorSet, i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfos[i]);
		}
		bufferWrites[12].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		pLogicalDevice->updateDescriptorSets(13, bufferWrites, 0, nullptr);

		VkCommandBuffer cmd_buf;
		gVkCommandPool.create(1, &cmd_buf);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(cmd_buf, &beginInfo);


		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelines[0]);
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1, &mDescriptorSet, 0, 0);
		vkCmdDispatch(cmd_buf, size, 1, 1);

		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelines[1]);
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1, &mDescriptorSet, 0, 0);
		vkCmdDispatch(cmd_buf, size == (size / 256) * 256 ? size / 256 : size / 256 + 1, 1, 1);


		if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		pLogicalDevice->executeCommands(pLogicalDevice->getComputeQueueIndex(), &cmd_buf, 1);

		gVkCommandPool.destroy(&cmd_buf, 1);

	}



};