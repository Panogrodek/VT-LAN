#version 460

layout(location = 0) in vec4 Color;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec2 WorldPos;

layout(location = 0) out vec4 FragColor;

struct Light {
    vec4 Position;
    vec4 Color;
    vec4 Direction;
    vec4 Data;
};

layout(set = 2, binding = 0) uniform sampler2D Texture;
layout(set = 2, binding = 1) uniform sampler2D NormalTexture;

layout(set = 2, binding = 2, std430) readonly buffer LightSSB {
    Light Lights[];
};

layout(set = 3, binding = 0) uniform LightUniforms {
    uint LightCount; // count of light in all the layers combined, for example if the object is on layer 3 
                     // we count all of the light on layers 0, 1, 2, 3 and then give it to light count i dont even know if this
                     // variable will be needed in the further version 
    uint LayerOrder;
};

void main() {
    vec4 albedo = texture(Texture, TexCoords) * Color;

    vec2 nxy = texture(NormalTexture, TexCoords).rg;
    vec2 nXY = nxy * 2.0 - 1.0;
    float nz2 = 1.0 - dot(nXY, nXY);
    float nz = nz2 > 0.0 ? sqrt(nz2) : 0.0;
    vec3 N = normalize(vec3(nXY, nz));

    vec3 lighting = vec3(0.08);

    for (uint i = 0u; i < LightCount; ++i) {
        vec2 lightXY = Lights[i].Position.xy;
        float radius3D = Lights[i].Position.w;
        vec3 lightCol = Lights[i].Color.rgb;

        float lightHeight = Lights[i].Position.z;
        if (lightHeight <= 0.0) {
            lightHeight = max(1.0, radius3D * 0.25);
        }

        vec3 toLight3 = vec3(lightXY - WorldPos, lightHeight);
        float dist3 = length(toLight3);

        if (dist3 >= radius3D) continue;

        vec3 L = normalize(toLight3);

        float NdotL = max(dot(N, L), 0.0);

        float att = 1.0 - (dist3 / radius3D);
        att = att * att;

        float intensity = 3.0;

        lighting += lightCol * (att * NdotL) * intensity;
    }

    vec3 finalRgb = albedo.rgb * lighting;
    FragColor = vec4(finalRgb, albedo.a);
}
