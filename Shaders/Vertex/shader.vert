#version 450

layout(set = 0, binding = 0) uniform GlobalUBO 
{
    mat4 view;
    mat4 proj;
} globalUBO;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    mat4 mvp = globalUBO.proj * globalUBO.view * pushConstants.model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    
    // simple pass-through to the fragment shader
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}