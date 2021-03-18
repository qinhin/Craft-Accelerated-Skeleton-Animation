#pragma once
#include "Global.h"

struct ModelFileVk
{
	using BufferMemory = vulkan::BufferMemory;
	BufferMemory mVertexBuffer;
	BufferMemory mIndexBuffer;


	void build(graphics::ModelFile* pFile)
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		mVertexBuffer = pLogicalDevice->createVertexBuffer(gVkCommandPool, pFile->mMeshBuffer.meshVertexData.data(), pFile->mMeshBuffer.meshVertexData.size() * sizeof(pFile->mMeshBuffer.meshVertexData[0]));
		mIndexBuffer = pLogicalDevice->createIndexBuffer(gVkCommandPool, pFile->mMeshBuffer.meshIndexData.data(), pFile->mMeshBuffer.meshIndexData.size() * sizeof(pFile->mMeshBuffer.meshIndexData[0]));
		
	}

	void release()
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		pLogicalDevice->destroy(mIndexBuffer);
		pLogicalDevice->destroy(mVertexBuffer);
	}

};