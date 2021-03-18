#pragma once
#include "Global.h"
#include "ModelFileVk.h"


class Scene
{
public:
	struct ModelData
	{
		graphics::ModelFile mFile;
		ModelFileVk mVkData;
		graphics::AABB mAABB;
	};


	struct ModelInstance
	{
		graphics::Transform mTransform;
		mat4 mModelMatrix;
		ModelData* mModelPtr;
		int mModelID;
		int mAnimationIdx;
		double mAnimationTime;
	};
	int mModelID = 0;
	int mInstanceID = 0;
	std::unordered_map<int, ModelData> mModels;
	std::unordered_map<int, ModelInstance> mModelInstances;

	int addModel(const char* path)
	{		
		ModelData model;
		auto success = graphics::importModel(path, model.mFile);
		if (!success)
			return -1;
		model.mVkData.build(&model.mFile);

		model.mAABB.mMin = { FLT_MAX, FLT_MAX, FLT_MAX };
		model.mAABB.mMax = { FLT_MIN, FLT_MIN, FLT_MIN };
		for (auto it : model.mFile.mMeshBuffer.meshVertexData)
		{
			for (int i = 0; i < 3; i++)
				if (it.mPosition[i] < model.mAABB.mMin[i])
					model.mAABB.mMin[i] = it.mPosition[i];
			for (int i = 0; i < 3; i++)
				if (it.mPosition[i] > model.mAABB.mMax[i])
					model.mAABB.mMax[i] = it.mPosition[i];
		}

		auto id = mModelID++;
		mModels.emplace(std::make_pair(id, model));
		return id;
	}

	int addInstance(int model)
	{
		ModelInstance instance;
		instance.mModelID = model;
		instance.mAnimationIdx = 0;
		instance.mAnimationTime = 0.0f;
		instance.mModelPtr = &mModels[model];
		instance.mTransform.mTranslate = vec3(0.0);

		auto& model_aabb = instance.mModelPtr->mAABB;
		auto scale = model_aabb.mMax - model_aabb.mMin;
		auto inverse_scale = 1.0f / math::min(scale.x, scale.y, scale.z);
		instance.mTransform.mScale = vec3(inverse_scale);

		auto id = mInstanceID++;
		mModelInstances.emplace(std::make_pair(id, instance));
		return id;
	}

	ModelInstance& getInstance(int id)
	{
		return mModelInstances[id];
	}

	void clearScene()
	{
		mModelInstances.clear();
	}

	void updateScene(double deltaTime)
	{
		for (auto& i : mModelInstances)
		{
			i.second.mAnimationTime += deltaTime;
			auto& mat = i.second.mModelMatrix;
			mat = math::scale(i.second.mTransform.mScale);
			mat = math::rotate(i.second.mTransform.mRotation, mat);
			mat = math::translate(i.second.mTransform.mTranslate, mat);
		}
	}

	void release()
	{
		clearScene();
		for (auto i : mModels)
			i.second.mVkData.release();
		mModels.clear();
	}

};