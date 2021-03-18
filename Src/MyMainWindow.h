#pragma once
#include "Global.h"
#include "Pipeline.h"
#include "Scene.h"
#include "Renderer.h"
#include "AnimationSlover.h"
#include "CraftEngine/graphics/guiext/CameraControllerWidget.h"

class MyMainWindow : public gui::MainWindow
{
private:

	gui::VulkanWidget* mVulkanWidget;
	graphics::extgui::CameraControllerWidget* mCameraControllerWidget;
	gui::FrameWidget* mFrameWidget;
	gui::DataBinderListWidget* mDataBinderListWidget;

	vulkan::LogicalDevice* m_pLogicalDevice;

	using BufferMemory = vulkan::BufferMemory;
	using ImageTexture = vulkan::ImageTexture;

	Pipeline mPipeline;
	Scene mScene;
	Renderer mRenderer;
	AnimationSlover mAnimationSlover;

public:


	MyMainWindow() 
	{
		setupUI();
		m_pLogicalDevice = vulkan::VulkanSystem::getLogicalDevice();
		gVkCommandPool = m_pLogicalDevice->createCommandPool();

		mPipeline.init();
		mRenderer.init(getWidth(), getHeight(), &mPipeline);
		mVulkanWidget->bindImage(mRenderer.mColorTarget.mView);
		mAnimationSlover.init();
		craft_engine_gui_connect_v2(this, resized, [=](const Size& size) {

			mVulkanWidget->setRect(Rect(Point(0), getSize()));
			this->mRenderer.resize(getWidth(), getHeight(), &this->mPipeline);
			mVulkanWidget->bindImage(mRenderer.mColorTarget.mView);
			mCameraControllerWidget->setRect(Rect(Point(0), getSize()));
		});
		craft_engine_gui_connect_v2(this, renderStarted, [=]() {
			if (gCurrentSceneSizeIndex != gSceneSizeIndex)
			{
				updateSceneSize();
				gCurrentSceneSizeIndex = gSceneSizeIndex;
			}
			this->mScene.updateScene(0.01667 * gAnimationSpeed);

			auto beg_time = std::chrono::system_clock::now();
			if (gAnimationSloverIndex == 0)
				this->mAnimationSlover.sloveWithCPU(&mScene);
			else if (gAnimationSloverIndex == 2)
				this->mAnimationSlover.sloveWithMultiCPU(&mScene);
			else
				this->mAnimationSlover.sloveWithGPU(&mScene);

			auto end_time = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - beg_time).count();
			double duration_ms = duration / double(1000000.0);
			gAnimationSlovingAccTime += duration_ms;
			gAnimationSlovingAccCount++;
			if (gAnimationSlovingAccTime >= 200.0f)
			{
				gAnimationSlovingTime = gAnimationSlovingAccTime / gAnimationSlovingAccCount;
				mDataBinderListWidget->updateData(5);
				gAnimationSlovingAccTime = 0;
				gAnimationSlovingAccCount = 0;
			}

			
			this->mRenderer.render(&mScene, &mPipeline, &mAnimationSlover);
		});

		createModels();
	}

	~MyMainWindow() 
	{
		mScene.release();
		mAnimationSlover.release();
		mRenderer.release();
		mPipeline.release();
		m_pLogicalDevice->destroy(gVkCommandPool);
		//std::cout << "a";
	}

protected:

	void setupUI()
	{
		mVulkanWidget = new gui::VulkanWidget(this);
		mVulkanWidget->setRect(Rect(Point(0), getSize()));
		mCameraControllerWidget = new graphics::extgui::CameraControllerWidget(this);
		mCameraControllerWidget->setRect(Rect(Point(0), getSize()));
		mCameraControllerWidget->setCamera(&gCamera);
		mFrameWidget = new gui::FrameWidget(this);
		mFrameWidget->setRect(Rect(0,0,200,getHeight()).padding(10,10));
		mFrameWidget->setTitle(L"Console");
		mDataBinderListWidget = new gui::DataBinderListWidget(mFrameWidget);
		mDataBinderListWidget->setRect(Rect(Point(0), mFrameWidget->getAvailableSize()));
		craft_engine_gui_connect_v2(mFrameWidget, resized, [=](const Size& size) {
			mDataBinderListWidget->setRect(Rect(Point(0), mFrameWidget->getAvailableSize()));
		});
		mDataBinderListWidget->bindDouble(&gAnimationSpeed, L"Animaition Speed");
		mDataBinderListWidget->setDataFormat(0, L"%.2f");
		mDataBinderListWidget->bindColor(&gClearColor, L"Clear Color");
		mDataBinderListWidget->bindColor(&gObjectColor, L"Object Color");
		mDataBinderListWidget->bindInt(&gSceneSizeIndex, { L"10x10=100",L"20x20=400",L"40x40=1600",L"50x50=2500",L"70*70=4900",L"100x100=10000", }, L"Scene Size");
		mDataBinderListWidget->bindIntEx(&gAnimationSloverIndex, { L"CPU(single-thread)",L"GPU(instance-batch)",L"CPU(multi-thread)" }, L"Animation Slover");
		mDataBinderListWidget->bindFloat(&gAnimationSlovingTime,  L"Animation Sloving Cost(ms)");
		mDataBinderListWidget->setDataFormat(5, L"%.2f");

	}


	void createModels()
	{

		mScene.addModel("./models/pd.X");
		mScene.addModel("./models/man.X");
		mAnimationSlover.buildSceneGpuAux(&mScene);

		updateSceneSize();
	}

	void updateSceneSize()
	{
		mScene.clearScene();
		int k = 0;
		switch (gSceneSizeIndex)
		{
		case 0: k = 5; break;
		case 1: k = 10; break;
		case 2: k = 20; break;
		case 3: k = 25; break;
		case 4: k = 35; break;
		case 5: k = 50; break;
		}

		for (int i = -k; i < k; i++)
		{
			for (int j = -k; j < k; j++)
			{
				vec3 pos = vec3(i * 2, 0, j * 2);
				auto id = mScene.addInstance(rand() % 2);
				auto& ins = mScene.getInstance(id);
				ins.mTransform.mTranslate = pos;
				ins.mAnimationTime = rand() / double(RAND_MAX) * 100.0f;
				ins.mAnimationIdx = rand() % 5;
			}
		}

	}

};