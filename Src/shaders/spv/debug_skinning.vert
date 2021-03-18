#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTextcoord;
layout (location = 3) in vec2 inTextcoord2;
layout (location = 4) in vec3 inTangent;
layout (location = 5) in uvec4 inBoneIDs;
layout (location = 6) in vec4 inBoneWeights;

layout (location = 0) out vec2 outTextcoord;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outPosition;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out vec3 outBitTangent;
out gl_PerVertex 
{
	vec4 gl_Position;   
};

layout (push_constant, binding = 0) uniform PushContants{ int pcBoneCount; };
layout (set = 0, binding = 0) uniform CameraProperties 
{
	mat4 view;
	mat4 proj;
	vec4 viewPos;
	vec4 surfaceColor;
	vec4 lightDir;
	vec4 lightColor;
	vec4 ambientColor;
	float shininess;
}cam;

layout (set = 1, binding = 0) readonly buffer ModelMatriceBuffer{ mat4 bufModelMatrice[]; };
layout (set = 1, binding = 1) readonly buffer BoneMatriceBuffer{ mat4 bufBoneMatrice[]; };

void main(void)
{
	int boneMatOffset = pcBoneCount * gl_InstanceIndex;

	mat4 BoneMatrix = bufBoneMatrice[boneMatOffset + inBoneIDs[0]] * inBoneWeights[0];
	BoneMatrix += bufBoneMatrice[boneMatOffset + inBoneIDs[1]] * inBoneWeights[1];
	BoneMatrix += bufBoneMatrice[boneMatOffset + inBoneIDs[2]] * inBoneWeights[2];
	BoneMatrix += bufBoneMatrice[boneMatOffset + inBoneIDs[3]] * inBoneWeights[3];


	vec4 world_space_pos = bufModelMatrice[gl_InstanceIndex] * BoneMatrix * vec4(inPosition, 1.0);
	outPosition = world_space_pos.xyz;
	vec4 screen_space_pos = cam.proj * cam.view * world_space_pos;
	gl_Position = screen_space_pos;

	outTextcoord = inTextcoord;

	mat3 normal_matrix = transpose(inverse(mat3(bufModelMatrice[gl_InstanceIndex]) * mat3(BoneMatrix)));
	outNormal = normal_matrix * inNormal;
	outTangent = normal_matrix * inTangent;
	outBitTangent = cross(outTangent, outNormal);
}

