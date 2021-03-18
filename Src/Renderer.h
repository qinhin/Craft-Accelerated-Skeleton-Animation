#pragma once
#include "Global.h"
#include "Pipeline.h"
#include "Scene.h"
#include "AnimationSlover.h"
class Renderer
{
public:
	using ImageTexture = vulkan::ImageTexture;
	using BufferMemory = vulkan::BufferMemory;
	int mWidth, mHeight;
	ImageTexture mColorTarget;
	ImageTexture mDepthTarget;
	VkFramebuffer mFramebuffer;


	BufferMemory mUniformBuffer;
	vulkan::DescriptorPool mUniformBufferDescriptorPool;
	VkDescriptorSetLayout mUniformBufferDescriptorSetLayout;
	VkDescriptorSet mUniformBufferDescriptorSet;


	void init(int w, int h, Pipeline* pipeline)
	{
		mWidth = w;
		mHeight = h;

		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		mColorTarget = pLogicalDevice->createColorAttachmentImage(mWidth, mHeight, vulkan::ImageFormat::format_rgba8_unorm);
		mDepthTarget = pLogicalDevice->createDepthAttachmentImage(mWidth, mHeight, vulkan::ImageFormat::format_d32_sfloat);

		auto attachments = {
			mColorTarget.mView,
			mDepthTarget.mView,
		};
		mFramebuffer = pLogicalDevice->createFramebuffer(
			pipeline->mRenderPass,
			attachments.size(),
			attachments.begin(),
			mWidth,
			mHeight,
			1);


		mUniformBuffer = pLogicalDevice->createUniformBuffer(sizeof(UniformBuffer));
		gUboData.shininess = 32.0f;
		gUboData.lightColor = vec4(0.5f);
		gUboData.ambientColor = vec4(0.2f);
		gUboData.lightDir.xyz = gCamera.getFrontDir();
		gUboData.surfaceColor = vec4(0.5, 0.2, 0.1f, 0.0f);
		VkDescriptorSetLayoutBinding bindings0[1] = {
			vulkan::vkt::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr),
		};
		mUniformBufferDescriptorSetLayout = pLogicalDevice->createDescriptorSetLayout(bindings0, 1);
		VkDescriptorPoolSize pool_size = vulkan::vkt::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
		mUniformBufferDescriptorPool = pLogicalDevice->createDescriptorPool(&pool_size, 1, 1);
		mUniformBufferDescriptorPool.create(&mUniformBufferDescriptorSetLayout, &mUniformBufferDescriptorSet, 1);
		VkDescriptorBufferInfo bufferInfos[] = {vulkan::vkt::descriptorBufferInfo(mUniformBuffer.mBuffer, 0, mUniformBuffer.mSize),};
		VkWriteDescriptorSet bufferWrites[] = {vulkan::vkt::writeDescriptorSet(mUniformBufferDescriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfos[0]),};
		pLogicalDevice->updateDescriptorSets(1, bufferWrites, 0, nullptr); 
	}

	void resize(int w, int h, Pipeline* pipeline)
	{
		release();
		init(w, h, pipeline);
	}

	void render(Scene* scene, Pipeline* pipeline, AnimationSlover* animationSlover)
	{

		gUboData.surfaceColor = vec4(gObjectColor) / 255.0f;

		gUboData.lightDir.xyz = gCamera.getFrontDir();
		gUboData.view = gCamera.matrices.view;
		gUboData.proj = gCamera.matrices.perspective;
		gUboData.viewPos.xyz = gCamera.getPosition();
		
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mUniformBuffer.mBuffer, &gUboData, sizeof(UniformBuffer), 0);


		VkCommandBuffer cmd_buf;
		gVkCommandPool.create(1, &cmd_buf);


		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkClearValue clearColor[2];
		clearColor[0].color.float32[0] = gClearColor[0] / 255.0f;
		clearColor[0].color.float32[1] = gClearColor[1] / 255.0f;
		clearColor[0].color.float32[2] = gClearColor[2] / 255.0f;
		clearColor[0].color.float32[3] = 1.0;
		clearColor[1].depthStencil.depth = 1.0;
		clearColor[1].depthStencil.stencil = 0;

		vkBeginCommandBuffer(cmd_buf, &beginInfo);
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = pipeline->mRenderPass;
		renderPassInfo.framebuffer = mFramebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { (uint32_t)mWidth, (uint32_t)mHeight };
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColor;

		VkViewport viewport = { 0.0f, 0.0f, (float)mWidth, (float)mHeight, 0.0f, 1.0f };
		vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
		VkRect2D scissor = { 0, 0, mWidth, mHeight };
		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
		//vkCmdSetLineWidth(cmd_buf, 1.0f);

		vkCmdBeginRenderPass(cmd_buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->mPipeline);

		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->mPipelineLayout, 0, 1, &mUniformBufferDescriptorSet, 0, nullptr);

		for (auto& i : animationSlover->mAuxData)
		{
			auto& model_data = scene->mModels[i.first];
			auto& aux_data = i.second;
			if (aux_data.mModelMatriceStagingBuffer.size() == 0)
				continue;
					
			vkCmdPushConstants(cmd_buf, pipeline->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(int), &aux_data.mNumBonePerModel);
			vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->mPipelineLayout, 1, 1, &aux_data.mDescriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(cmd_buf, 0, 1, &model_data.mVkData.mVertexBuffer.mBuffer, offsets);

			// 一次绘制，不考虑贴图材质。需把importModel修改为无偏移的索引的导入方式
			//vkCmdBindIndexBuffer(cmd_buf, model_data.mVkData.mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);
			//vkCmdDrawIndexed(cmd_buf, model_data.mFile.mMeshBuffer.meshIndexData.size(), aux_data.mModelMatriceStagingBuffer.size(), 0, 0, 0);

			// 分mesh绘制（默认）
			for (auto& m : model_data.mFile.mMeshBuffer.meshData)
			{
				vkCmdBindIndexBuffer(cmd_buf, model_data.mVkData.mIndexBuffer.mBuffer, m.vertexIndexOffset * sizeof(int), VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(cmd_buf, m.vertexIndexCount, aux_data.mModelMatriceStagingBuffer.size(), 0, m.vertexBase, 0);
			}
		}

		vkCmdEndRenderPass(cmd_buf);
		if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}


		pLogicalDevice->executeCommands(pLogicalDevice->getGraphicsQueueIndex(), &cmd_buf, 1);

		gVkCommandPool.destroy(&cmd_buf);
	}

	void release()
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();

		pLogicalDevice->destroy(mUniformBufferDescriptorPool);
		pLogicalDevice->destroyDescriptorSetLayout(mUniformBufferDescriptorSetLayout);
		pLogicalDevice->destroy(mUniformBuffer);		
		pLogicalDevice->destroyFramebuffer(mFramebuffer);
		pLogicalDevice->destroy(mDepthTarget);
		pLogicalDevice->destroy(mColorTarget);
	}

};