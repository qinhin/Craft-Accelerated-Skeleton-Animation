#pragma once
//#define CRAFT_ENGINE_USING_WORLD_SPACE_RIGHT_HAND

#include "CraftEngine/gui/Gui.h"
using namespace CraftEngine;
using math::vec2;
using math::vec3;
using math::vec4;
using math::mat2;
using math::mat3;
using math::mat4;
using gui::Rect;
using gui::Size;
using gui::Point;
using gui::String;


#include "CraftEngine/gui/widgetsext/VulkanWidget.h"


#include "CraftEngine/graphics/Camera.h"
using graphics::Camera;
Camera gCamera;
#include "CraftEngine/graphics/ModelImport.h"


vulkan::CommandPool gVkCommandPool;


struct UniformBuffer
{
	mat4 view;
	mat4 proj;
	vec4 viewPos;
	vec4 surfaceColor;
	vec4 lightDir;
	vec4 lightColor;
	vec4 ambientColor;
	float shininess;
}gUboData;


double gAnimationSpeed = 100.0f;
gui::Color gClearColor = gui::Color(69, 97, 155, 255);
gui::Color gObjectColor = gui::Color(142, 70, 80, 255);
int gSceneSizeIndex = 0;
int gCurrentSceneSizeIndex = 0;
int gAnimationSloverIndex = 0;

float gAnimationSlovingTime = 0.0;
float gAnimationSlovingAccTime = 0;
int gAnimationSlovingAccCount = 0;