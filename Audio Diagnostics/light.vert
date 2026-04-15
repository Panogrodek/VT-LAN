#version 460

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec4 Color;

layout(location = 0) out vec4 iColor;
layout(location = 1) out vec2 iTexCoords;
layout(location = 2) out vec2 iWorldPos;

layout(set = 1, binding = 0) uniform Uniforms {
    mat4 Projection;
    mat4 View;
    mat4 Model;
};

void main() {
    vec4 worldPos = Model * vec4(Position, 1.0);

    gl_Position = Projection * View * worldPos;

    iColor     = Color;
    iTexCoords = TexCoords;
    iWorldPos  = worldPos.xy;
}
