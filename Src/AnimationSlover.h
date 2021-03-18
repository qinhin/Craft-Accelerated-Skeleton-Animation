#pragma once
#include "Global.h"
#include "Scene.h"
#include "CraftEngine/graphics/AnimationSlover.h"
#include "ComputePipeline.h"
class AnimationSlover
{
public:
	using BufferMemory = vulkan::BufferMemory;
	using CPUAnimationSlover = graphics::AnimationSlover;
	struct ModelRenderAuxData
	{
		std::vector<mat4> mBoneMatriceStagingBuffer;
		std::vector<mat4> mModelMatriceStagingBuffer;
		VkDescriptorSet mDescriptorSet;
		int mBoneMatriceOffset;
		int mModelMatriceOffset;
		int mBoneMatriceSize;
		int mModelMatriceSize;
		int mNumBonePerModel;		
	};
	int mCapacity[2];
	int mSize[2];
	BufferMemory mBoneMatriceData;
	BufferMemory mModelMatriceData;
	vulkan::DescriptorPool mDescriptorPool;
	VkDescriptorSetLayout mDescriptorSetLayout;	
	std::unordered_map<int, ModelRenderAuxData> mAuxData;

	// GPU
	ComputePipeline mComputePipeline;

	// CPU - MT
	core::ThreadPool mThreadPool;

	struct SceneGpuAuxData
	{
		
		struct alignas(16) ModelInfo
		{
			int nodeOffset;
			int nodeCount;
			int childrenOffset;
			int childrenCount;
			int boneOffset;
			int boneCount;
			int animationOffset;
			int animationCount;
			int channelOffset;
			int channelCount;
			int posKeyOffset;
			int posKeyCount;
			int rotKeyOffset;
			int rotKeyCount;
			int scaKeyOffset;
			int scaKeyCount;
		};
		struct alignas(16) NodeInfo
		{
			mat4 transform;
			int childrenOffset;
			int childrenCount;
			int boneIndex;
		};
		//constexpr int a = sizeof(NodeInfo);
		struct alignas(16) BoneInfo
		{
			mat4 offsetMatrix;
			int nodeIndex;
		};
		struct alignas(16) AnimationInfo
		{
			int channelOffset;
			int channelCount;
			float duration;
			float ticksPerSecond;
		};
		struct alignas(16) ChannelInfo
		{
			int nodeIndex;
			int boneIndex;
			int posKeyOffset;
			int posKeyCount;
			int rotKeyOffset;
			int rotKeyCount;
			int scaKeyOffset;
			int scaKeyCount;
		};
		struct alignas(16) RotKey
		{
			math::quat rotation;
			float time;
		};
		struct alignas(16) InstanceInfo
		{
			int localTransformOffset;
			int localTransformCount;
			int globalTransformOffset;
			int globalTransformCount;
			int modelID;
			int animationID;
			float animationTime;
		};
		BufferMemory mBuffers[11];
		BufferMemory mUniformBuffer;
		int mCapacity9 = 0;
		int mCapacity10 = 0;
		bool mReady = false;
	} mSceneGpuAuxData;


	void init()
	{

		// CPU
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		VkDescriptorPoolSize pool_size = vulkan::vkt::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024);
		mDescriptorPool = pLogicalDevice->createDescriptorPool(&pool_size, 1, 1024 / 2);

		VkDescriptorSetLayoutBinding bindings1[2] = {
			vulkan::vkt::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr),
			vulkan::vkt::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr),
		};
		mDescriptorSetLayout = pLogicalDevice->createDescriptorSetLayout(bindings1, 2);

		mCapacity[1] = mCapacity[0] = 1024;	
		mModelMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[0] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mBoneMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[1] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		
		// GPU
		mComputePipeline.init();
		mSceneGpuAuxData.mUniformBuffer = pLogicalDevice->createUniformBuffer(sizeof(int));

		// CPU - MT
		mThreadPool.init(std::thread::hardware_concurrency());
	}


	void sloveWithCPU(Scene* scene)
	{
		CPUAnimationSlover slover;

		mSize[0] = 0;
		mSize[1] = 0;
		for (auto& aux_data : mAuxData)
		{
			aux_data.second.mBoneMatriceStagingBuffer.resize(0);
			aux_data.second.mModelMatriceStagingBuffer.resize(0);
		}

		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		for (auto& instance : scene->mModelInstances)
		{
			auto data = mAuxData.find(instance.second.mModelID);
			if (data == mAuxData.end())
			{
				mAuxData.emplace(std::make_pair(instance.second.mModelID, ModelRenderAuxData()));
				data = mAuxData.find(instance.second.mModelID);
				data->second.mNumBonePerModel = instance.second.mModelPtr->mFile.mBoneBuffer.boneData.size();		
				mDescriptorPool.create(&mDescriptorSetLayout, &data->second.mDescriptorSet, 1);
			}
			auto& aux_data_ref = data->second;

			slover.bindModel(&instance.second.mModelPtr->mFile);
			slover.evaluate(instance.second.mAnimationTime, instance.second.mAnimationIdx);
			for (auto& m : slover.mBoneTransformMatrices)
				aux_data_ref.mBoneMatriceStagingBuffer.push_back(m);
			aux_data_ref.mModelMatriceStagingBuffer.push_back(instance.second.mModelMatrix);

			mSize[0] += 1;
			mSize[1] += data->second.mNumBonePerModel;
		}

		if (mSize[0] > mCapacity[0])
		{
			mCapacity[0] = mSize[0];
			pLogicalDevice->destroy(mModelMatriceData);
			mModelMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[0] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);		
		}
		if (mSize[1] > mCapacity[1])
		{
			mCapacity[1] = mSize[1];
			pLogicalDevice->destroy(mBoneMatriceData);
			mBoneMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[1] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}

		size_t offsets[2] = { 0,0 };
		for (auto& aux_data : mAuxData)
		{
			size_t sizes[2] = { sizeof(math::mat4) * aux_data.second.mModelMatriceStagingBuffer.size(), sizeof(math::mat4) * aux_data.second.mBoneMatriceStagingBuffer.size() };

			VkDescriptorBufferInfo bufferInfos[2] = {
				vulkan::vkt::descriptorBufferInfo(mModelMatriceData.mBuffer, offsets[0], sizes[0]),
				vulkan::vkt::descriptorBufferInfo(mBoneMatriceData.mBuffer, offsets[1], sizes[1]),
			};
			VkWriteDescriptorSet bufferWrites[2] = {
				vulkan::vkt::writeDescriptorSet(aux_data.second.mDescriptorSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfos[0]),
				vulkan::vkt::writeDescriptorSet(aux_data.second.mDescriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfos[1]),
			};
			pLogicalDevice->updateDescriptorSets(2, bufferWrites, 0, nullptr);


			pLogicalDevice->copyDataToBuffer(gVkCommandPool, mModelMatriceData.mBuffer, aux_data.second.mModelMatriceStagingBuffer.data(), sizes[0], offsets[0]);
			pLogicalDevice->copyDataToBuffer(gVkCommandPool, mBoneMatriceData.mBuffer, aux_data.second.mBoneMatriceStagingBuffer.data(), sizes[1], offsets[1]);
			aux_data.second.mModelMatriceOffset = offsets[0];
			aux_data.second.mBoneMatriceOffset = offsets[1];
			offsets[0] += sizes[0];
			offsets[1] += sizes[1];
		}

	}


	void sloveWithMultiCPU(Scene* scene)
	{
		

		mSize[0] = 0;
		mSize[1] = 0;
		for (auto& aux_data : mAuxData)
		{
			aux_data.second.mBoneMatriceStagingBuffer.resize(0);
			aux_data.second.mModelMatriceStagingBuffer.resize(0);
		}

		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		std::mutex mutex_shared, mutex;
		auto thread_count = mThreadPool.threadCount();
		for (int i = 0; i < thread_count; i++)
		{
			mThreadPool.push([this, i, thread_count, &scene, &mutex]() {
				int idx = 0;
				CPUAnimationSlover slover;
				for (const auto& instance : scene->mModelInstances)
				{
					if (idx % thread_count == i)
					{
						//mutex.lock();
						auto data = mAuxData.find(instance.second.mModelID);
						if (data == mAuxData.end())
						{
							mAuxData.emplace(std::make_pair(instance.second.mModelID, ModelRenderAuxData()));
							data = mAuxData.find(instance.second.mModelID);
							data->second.mNumBonePerModel = instance.second.mModelPtr->mFile.mBoneBuffer.boneData.size();
							mDescriptorPool.create(&mDescriptorSetLayout, &data->second.mDescriptorSet, 1);
						}
						//mutex.unlock();
						auto& aux_data_ref = data->second;

						slover.bindModel(&instance.second.mModelPtr->mFile);
						slover.evaluate(instance.second.mAnimationTime, instance.second.mAnimationIdx);

						mutex.lock();

						for (auto& m : slover.mBoneTransformMatrices)
							aux_data_ref.mBoneMatriceStagingBuffer.push_back(m);
						aux_data_ref.mModelMatriceStagingBuffer.push_back(instance.second.mModelMatrix);

						mSize[0] += 1;
						mSize[1] += data->second.mNumBonePerModel;
						mutex.unlock();
					}
					idx++;
				}

			}, i);
		}
		mThreadPool.wait();

		if (mSize[0] > mCapacity[0])
		{
			mCapacity[0] = mSize[0];
			pLogicalDevice->destroy(mModelMatriceData);
			mModelMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[0] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}
		if (mSize[1] > mCapacity[1])
		{
			mCapacity[1] = mSize[1];
			pLogicalDevice->destroy(mBoneMatriceData);
			mBoneMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[1] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}

		size_t offsets[2] = { 0,0 };
		for (auto& aux_data : mAuxData)
		{
			size_t sizes[2] = { sizeof(math::mat4) * aux_data.second.mModelMatriceStagingBuffer.size(), sizeof(math::mat4) * aux_data.second.mBoneMatriceStagingBuffer.size() };

			VkDescriptorBufferInfo bufferInfos[2] = {
				vulkan::vkt::descriptorBufferInfo(mModelMatriceData.mBuffer, offsets[0], sizes[0]),
				vulkan::vkt::descriptorBufferInfo(mBoneMatriceData.mBuffer, offsets[1], sizes[1]),
			};
			VkWriteDescriptorSet bufferWrites[2] = {
				vulkan::vkt::writeDescriptorSet(aux_data.second.mDescriptorSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfos[0]),
				vulkan::vkt::writeDescriptorSet(aux_data.second.mDescriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfos[1]),
			};
			pLogicalDevice->updateDescriptorSets(2, bufferWrites, 0, nullptr);


			pLogicalDevice->copyDataToBuffer(gVkCommandPool, mModelMatriceData.mBuffer, aux_data.second.mModelMatriceStagingBuffer.data(), sizes[0], offsets[0]);
			pLogicalDevice->copyDataToBuffer(gVkCommandPool, mBoneMatriceData.mBuffer, aux_data.second.mBoneMatriceStagingBuffer.data(), sizes[1], offsets[1]);
			aux_data.second.mModelMatriceOffset = offsets[0];
			aux_data.second.mBoneMatriceOffset = offsets[1];
			offsets[0] += sizes[0];
			offsets[1] += sizes[1];
		}

	}


	std::vector<SceneGpuAuxData::InstanceInfo> instanceInfos;
	void sloveWithGPU(Scene* scene)
	{
		mSize[0] = 0;
		mSize[1] = 0;
		for (auto& aux_data : mAuxData)
		{
			aux_data.second.mBoneMatriceStagingBuffer.resize(0);
			aux_data.second.mModelMatriceStagingBuffer.resize(0);
		}

		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();

		instanceInfos.clear();

		int offsets[3] = {0,0,0};
		for (auto& model : scene->mModels)
		{
			auto data = mAuxData.find(model.first);
			if (data == mAuxData.end())
			{
				mAuxData.emplace(std::make_pair(model.first, ModelRenderAuxData()));
				data = mAuxData.find(model.first);
				data->second.mNumBonePerModel = model.second.mFile.mBoneBuffer.boneData.size();
				mDescriptorPool.create(&mDescriptorSetLayout, &data->second.mDescriptorSet, 1);
			}
			auto& aux_data_ref = data->second;
			aux_data_ref.mModelMatriceOffset = offsets[0];
			aux_data_ref.mBoneMatriceOffset = offsets[1];
			for (auto& instance : scene->mModelInstances)
			{
				if (instance.second.mModelID != model.first)
					continue;
				SceneGpuAuxData::InstanceInfo instanceInfo;
				instanceInfo.animationID = instance.second.mAnimationIdx;
				instanceInfo.animationTime = instance.second.mAnimationTime;
				instanceInfo.modelID = instance.second.mModelID;
				instanceInfo.localTransformCount = model.second.mFile.mAnimationBuffer.animationInfoData[instanceInfo.animationID].channelCount;
				instanceInfo.localTransformOffset = offsets[2];
				instanceInfo.globalTransformCount = aux_data_ref.mNumBonePerModel;
				instanceInfo.globalTransformOffset = offsets[1];
				instanceInfos.push_back(instanceInfo);

				aux_data_ref.mModelMatriceStagingBuffer.push_back(instance.second.mModelMatrix);
				offsets[0] += 1;
				offsets[1] += aux_data_ref.mNumBonePerModel;
				offsets[2] += instanceInfo.localTransformCount;
			}
			aux_data_ref.mModelMatriceSize = offsets[0] - aux_data_ref.mModelMatriceOffset;
			aux_data_ref.mBoneMatriceSize = offsets[1] - aux_data_ref.mBoneMatriceOffset;
		}
		mSize[0] = offsets[0];
		mSize[1] = offsets[1];

		if (mSize[0] > mCapacity[0])
		{
			mCapacity[0] = mSize[0];
			pLogicalDevice->destroy(mModelMatriceData);
			mModelMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[0] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}
		if (mSize[1] > mCapacity[1])
		{
			mCapacity[1] = mSize[1];
			pLogicalDevice->destroy(mBoneMatriceData);
			mBoneMatriceData = pLogicalDevice->createDeviceBuffer(mCapacity[1] * sizeof(math::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		}

		if (mSceneGpuAuxData.mCapacity9 < instanceInfos.size())
		{
			mSceneGpuAuxData.mCapacity9 = instanceInfos.size();
			if(mSceneGpuAuxData.mCapacity9 > 0)
				pLogicalDevice->destroy(mSceneGpuAuxData.mBuffers[9]);
			mSceneGpuAuxData.mBuffers[9] = pLogicalDevice->createDeviceBuffer(mSceneGpuAuxData.mCapacity9 * sizeof(SceneGpuAuxData::InstanceInfo), 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}
		if (mSceneGpuAuxData.mCapacity10 < offsets[2])
		{
			mSceneGpuAuxData.mCapacity10 = offsets[2];
			if (mSceneGpuAuxData.mCapacity10 > 0)
				pLogicalDevice->destroy(mSceneGpuAuxData.mBuffers[10]);
			mSceneGpuAuxData.mBuffers[10] = pLogicalDevice->createDeviceBuffer(mSceneGpuAuxData.mCapacity10 * sizeof(math::mat4),
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		}
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[9].mBuffer, instanceInfos.data(), instanceInfos.size() * sizeof(SceneGpuAuxData::InstanceInfo), 0);

		BufferMemory buffers[13];
		for (int i = 0; i < 11; i++) buffers[i] = mSceneGpuAuxData.mBuffers[i];
		buffers[11] = mBoneMatriceData;
		buffers[12] = mSceneGpuAuxData.mUniformBuffer;

		int instance_size = instanceInfos.size();
		mSceneGpuAuxData.mUniformBuffer.write(&instance_size, sizeof(int));
		mComputePipeline.compute(buffers, instanceInfos.size());

		for (auto& aux_data : mAuxData)
		{
			VkDescriptorBufferInfo bufferInfos[2] = {
				vulkan::vkt::descriptorBufferInfo(mModelMatriceData.mBuffer, sizeof(math::mat4) * aux_data.second.mModelMatriceOffset, sizeof(math::mat4) * aux_data.second.mModelMatriceSize),
				vulkan::vkt::descriptorBufferInfo(mBoneMatriceData.mBuffer,  sizeof(math::mat4) * aux_data.second.mBoneMatriceOffset, sizeof(math::mat4) * aux_data.second.mBoneMatriceSize),
			};
			VkWriteDescriptorSet bufferWrites[2] = {
				vulkan::vkt::writeDescriptorSet(aux_data.second.mDescriptorSet, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfos[0]),
				vulkan::vkt::writeDescriptorSet(aux_data.second.mDescriptorSet, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfos[1]),
			};
			pLogicalDevice->updateDescriptorSets(2, bufferWrites, 0, nullptr);
			pLogicalDevice->copyDataToBuffer(gVkCommandPool, mModelMatriceData.mBuffer, aux_data.second.mModelMatriceStagingBuffer.data(), sizeof(math::mat4) * aux_data.second.mModelMatriceSize, sizeof(math::mat4) * aux_data.second.mModelMatriceOffset);
		}

		//pLogicalDevice->waitQueue(pLogicalDevice->getComputeQueueIndex());

		//std::vector<math::mat4> localTransform(mSceneGpuAuxData.mCapacity10);
		//pLogicalDevice->copyBufferToData(gVkCommandPool, localTransform.data(), mSceneGpuAuxData.mBuffers[10].mBuffer, mSceneGpuAuxData.mCapacity10 * sizeof(math::mat4), 0);
		//std::vector<math::mat4> globalTransform(mSize[1]);
		//pLogicalDevice->copyBufferToData(gVkCommandPool, globalTransform.data(), mBoneMatriceData.mBuffer, mSize[1] * sizeof(math::mat4), 0);

	}



	void buildSceneGpuAux(Scene* scene)
	{
		if (mSceneGpuAuxData.mReady == true)
			return;
		mSceneGpuAuxData.mReady = true;

		std::vector<SceneGpuAuxData::ModelInfo> modelInfos;
		std::vector<SceneGpuAuxData::NodeInfo> nodeInfos;
		std::vector<int> childrenInfos;
		std::vector<SceneGpuAuxData::BoneInfo> boneInfos;
		std::vector<SceneGpuAuxData::AnimationInfo> animationInfos;
		std::vector<SceneGpuAuxData::ChannelInfo> channelInfos;
		std::vector<vec4> posKeyInfos;
		std::vector<SceneGpuAuxData::RotKey> rotKeyInfos;
		std::vector<vec4> scaKeyInfos;

		modelInfos.resize(scene->mModels.size());
		for (auto& model : scene->mModels)
		{
			auto& modelInfo = modelInfos[model.first];
			auto& file = model.second.mFile;
			modelInfo.nodeOffset = nodeInfos.size();
			modelInfo.nodeCount = file.mNodeBuffer.nodeData.size();
			modelInfo.childrenOffset = childrenInfos.size();
			modelInfo.childrenCount = file.mNodeBuffer.nodeChildIndexData.size();
			modelInfo.boneOffset = boneInfos.size();
			modelInfo.boneCount = file.mBoneBuffer.boneData.size();
			modelInfo.animationOffset = animationInfos.size();
			modelInfo.animationCount = file.mAnimationBuffer.animationInfoData.size();
			modelInfo.channelOffset = channelInfos.size();
			modelInfo.channelCount = file.mAnimationBuffer.channelData.size();
			modelInfo.posKeyOffset = posKeyInfos.size();
			modelInfo.posKeyCount = file.mAnimationBuffer.positionKeyData.size();
			modelInfo.rotKeyOffset = rotKeyInfos.size();
			modelInfo.rotKeyCount = file.mAnimationBuffer.rotationKeyData.size();
			modelInfo.scaKeyOffset = scaKeyInfos.size();
			modelInfo.scaKeyCount = file.mAnimationBuffer.scalingKeyData.size();
			// nodes
			for (auto& node : file.mNodeBuffer.nodeData)
			{
				nodeInfos.push_back(SceneGpuAuxData::NodeInfo());
				auto& nodeInfo = nodeInfos.back();
				nodeInfo.transform = node.transform;
				nodeInfo.childrenOffset = node.childIndexOffset;
				nodeInfo.childrenCount = node.childIndexCount;
				nodeInfo.boneIndex = node.boneIndex;
			}
			// children
			for (auto& c : file.mNodeBuffer.nodeChildIndexData)
				childrenInfos.push_back(c);
			// bones
			for (auto& bone : file.mBoneBuffer.boneData)
			{
				boneInfos.push_back(SceneGpuAuxData::BoneInfo());
				auto& boneInfo = boneInfos.back();
				boneInfo.offsetMatrix = bone.offsetMatrix;
				boneInfo.nodeIndex = bone.nodeIndex;
			}
			// animations
			for (auto& animation : file.mAnimationBuffer.animationInfoData)
			{
				animationInfos.push_back(SceneGpuAuxData::AnimationInfo());
				auto& animationInfo = animationInfos.back();
				animationInfo.channelOffset = animation.channelOffset;
				animationInfo.channelCount = animation.channelCount;

				animationInfo.duration = animation.duration;
				animationInfo.ticksPerSecond = animation.ticksPerSecond;

				// channel
				for (auto c = 0; c < animationInfo.channelCount; c++)
				{
					auto& channel = file.mAnimationBuffer.channelData[animationInfo.channelOffset + c];
					channelInfos.push_back(SceneGpuAuxData::ChannelInfo());
					auto& channelInfo = channelInfos.back();
					channelInfo.boneIndex = channel.boneIndex;
					channelInfo.nodeIndex = channel.nodeIndex;
					channelInfo.posKeyOffset = channel.positionKeyOffset;
					channelInfo.posKeyCount = channel.positionKeyCount;
					channelInfo.rotKeyOffset = channel.rotationKeyOffset;
					channelInfo.rotKeyCount = channel.rotationKeyCount;
					channelInfo.scaKeyOffset = channel.scalingKeyOffset;
					channelInfo.scaKeyCount = channel.scalingKeyCount;

					for (int i = 0; i < channelInfo.posKeyCount; i++)
					{
						vec4 posKey;
						posKey.xyz = file.mAnimationBuffer.positionKeyData[channel.positionKeyOffset + i].position;
						posKey.w = file.mAnimationBuffer.positionKeyData[channel.positionKeyOffset + i].time;
						posKeyInfos.push_back(posKey);
					}
					for (int i = 0; i < channelInfo.rotKeyCount; i++)
					{
						SceneGpuAuxData::RotKey rotKey;
						rotKey.rotation = file.mAnimationBuffer.rotationKeyData[channel.rotationKeyOffset + i].rotation;
						rotKey.time = file.mAnimationBuffer.rotationKeyData[channel.rotationKeyOffset + i].time;
						rotKeyInfos.push_back(rotKey);
					}
					for (int i = 0; i < channelInfo.scaKeyCount; i++)
					{
						vec4 scaKey;
						scaKey.xyz = file.mAnimationBuffer.scalingKeyData[channel.scalingKeyOffset + i].scaling;
						scaKey.w = file.mAnimationBuffer.scalingKeyData[channel.scalingKeyOffset + i].time;
						scaKeyInfos.push_back(scaKey);
					}
				}
			}
		}

		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		mSceneGpuAuxData.mBuffers[0] = pLogicalDevice->createDeviceBuffer(modelInfos.size() * sizeof(modelInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[1] = pLogicalDevice->createDeviceBuffer(nodeInfos.size() * sizeof(nodeInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[2] = pLogicalDevice->createDeviceBuffer(childrenInfos.size() * sizeof(childrenInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[3] = pLogicalDevice->createDeviceBuffer(boneInfos.size() * sizeof(boneInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[4] = pLogicalDevice->createDeviceBuffer(animationInfos.size() * sizeof(animationInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[5] = pLogicalDevice->createDeviceBuffer(channelInfos.size() * sizeof(channelInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[6] = pLogicalDevice->createDeviceBuffer(posKeyInfos.size() * sizeof(posKeyInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[7] = pLogicalDevice->createDeviceBuffer(rotKeyInfos.size() * sizeof(rotKeyInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		mSceneGpuAuxData.mBuffers[8] = pLogicalDevice->createDeviceBuffer(scaKeyInfos.size() * sizeof(scaKeyInfos[0]), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[0].mBuffer, modelInfos.data(), modelInfos.size() * sizeof(modelInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[1].mBuffer, nodeInfos.data(), nodeInfos.size() * sizeof(nodeInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[2].mBuffer, childrenInfos.data(), childrenInfos.size() * sizeof(childrenInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[3].mBuffer, boneInfos.data(), boneInfos.size() * sizeof(boneInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[4].mBuffer, animationInfos.data(), animationInfos.size() * sizeof(animationInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[5].mBuffer, channelInfos.data(), channelInfos.size() * sizeof(channelInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[6].mBuffer, posKeyInfos.data(), posKeyInfos.size() * sizeof(posKeyInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[7].mBuffer, rotKeyInfos.data(), rotKeyInfos.size() * sizeof(rotKeyInfos[0]), 0);
		pLogicalDevice->copyDataToBuffer(gVkCommandPool, mSceneGpuAuxData.mBuffers[8].mBuffer, scaKeyInfos.data(), scaKeyInfos.size() * sizeof(scaKeyInfos[0]), 0);
	}




	void release()
	{
		auto pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();

		// GPU
		mComputePipeline.release();
		for (int i = 0; i < 11; i++)
		{
			pLogicalDevice->destroy(mSceneGpuAuxData.mBuffers[i]);
		}
		pLogicalDevice->destroy(mSceneGpuAuxData.mUniformBuffer);
		
		// CPU
		pLogicalDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
		pLogicalDevice->destroy(mBoneMatriceData);
		pLogicalDevice->destroy(mModelMatriceData);
		pLogicalDevice->destroy(mDescriptorPool);
		mAuxData.clear();
	}

};

