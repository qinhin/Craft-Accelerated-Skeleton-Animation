#version 450

layout (location = 0) in vec2 inTexcoord;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitTangent;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraProperties 
{
	mat4 view;
	mat4 proj;
	vec4 viewPos;
	vec4 surfaceColor;
	vec4 lightDir;
	vec4 lightColor;
	vec4 ambientColor;
	float shininess;
} cam;

void main() 
{
	vec3 normal = normalize(inNormal);
	vec3 viewDir = normalize(cam.viewPos.xyz - inPosition);
	vec3 lightDir = normalize(-cam.lightDir.xyz);
	//vec3 lightDir = normalize(cam.lightDir.xyz - inPosition);

	vec3 halfVector = normalize(lightDir + viewDir);

	vec3 ambient = cam.ambientColor.xyz;
	vec3 diffuse = max(0.0, dot(normal, lightDir)) * cam.lightColor.xyz;
	vec3 specular = pow(max(0.0, dot(normal, halfVector)), cam.shininess) * cam.lightColor.xyz;


	vec3 color = cam.surfaceColor.xyz * (ambient + diffuse + specular);
	outColor = vec4(color, 1.0);

}

