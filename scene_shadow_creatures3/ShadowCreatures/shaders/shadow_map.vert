#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aInstanceOffset;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform int useInstancing;

void main()
{
    vec3 worldPos = vec3(model * vec4(aPos, 1.0));
    if (useInstancing == 1) {
        worldPos += aInstanceOffset;
    }
    gl_Position = lightSpaceMatrix * vec4(worldPos, 1.0);
}
