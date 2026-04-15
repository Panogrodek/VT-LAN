#version 460

#define MAX_LIGHTS_PER_CAMERA 64

layout(location = 0) in vec4 Color;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec2 WorldPos;

layout(location = 0) out vec4 FragColor;

const float PIXELIZATION_SIZE            = 1.75;
const int   QUANTIZATION_INTENSITY_STEPS = 32;
const float SPECULAR_STRENGTH            = 0.2;
const float SPECULAR_POWER               = 64.0;
const float AMBIENT                      = 0.05;
const float DITHER_STRENGTH              = 0.75;

struct Light {
    vec4 Position;  
    vec4 Color;     
    vec4 Direction;
    vec4 Data;
};

struct LightIndices {
    uint Indices[MAX_LIGHTS_PER_CAMERA];
    uint LightCount;
};

layout(set = 2, binding = 0) uniform sampler2D Texture;
layout(set = 2, binding = 1) uniform sampler2D NormalTexture;

layout(set = 2, binding = 2, std430) readonly buffer LightSSB {
    Light Lights[];
};

layout(set = 2, binding = 3, std430) readonly buffer LightIndicesSSB {
    LightIndices LightIndicesArray[];
};

layout(set = 3, binding = 0) uniform LightUniforms {
    uint CameraIndex;
    uint ObjectLayer;
};

float Bayer4(vec2 p) {
    int x = int(mod(p.x, 4.0));
    int y = int(mod(p.y, 4.0));
    int idx = x + y * 4;
    int mat[16] = int[16]
    (
        0,   8,  2, 10,
        12,  4, 14,  6,
        3,  11,  1,  9,
        15,  7, 13,  5
    );

    return float(mat[idx]) / 16.0;
}

float LightAttenuationLinear(float dist, float radius) {
    if (radius <= 0.0) return 0.0;
    float t = 1.0 - (dist / radius);
    return max(0.0, t);
}

vec3 ShadePointLight(Light L, vec2 blockCenter, vec3 baseColor, vec3 N, vec3 viewDir, float threshold, float invSteps) {
    vec2 lpos    = L.Position.xy;
    float radius = L.Position.w;
    vec2 delta2  = lpos - blockCenter;
    float dist2d = length(delta2);

    float attenuation = LightAttenuationLinear(dist2d, radius);
    float lightZ = (abs(L.Position.z) < 1e-6) ? 1.0 : L.Position.z;

    vec3 Ldir   = normalize(vec3(delta2, lightZ));
    float NdotL = max(dot(N, Ldir), 0.0);
    float raw   = NdotL * attenuation;

    vec3 H = normalize(Ldir + viewDir);

    // quantize
    float stepped = floor(raw * float(QUANTIZATION_INTENSITY_STEPS)) * invSteps;
    float next = clamp(stepped + invSteps, 0.0, 1.0);
    float frac = (raw - stepped) / invSteps;

    float dithered = (frac > threshold) ? next : stepped;
    float intensity = mix(stepped, dithered, DITHER_STRENGTH);

    // diffuse
    vec3 diffuse = baseColor * L.Color.rgb * intensity;

    // specular    
    float NdotH   = max(dot(N, H), 0.0);
    float specVal = SPECULAR_STRENGTH * pow(NdotH, SPECULAR_POWER) * attenuation;
    vec3 specular = specVal * L.Color.rgb;

    return diffuse + specular;
}

void main() {
    vec4 albedo = texture(Texture, TexCoords) * Color;
    vec3 baseColor = albedo.rgb;

    vec3 normal = texture(NormalTexture, TexCoords).rgb * 2.0 - 1.0;
    normal.y = -normal.y;

    vec3 N = normalize(normal);
    vec3 V = vec3(0.0, 0.0, 1.0); // view vector for specular lighting

    vec2 blockCenter = floor(WorldPos / PIXELIZATION_SIZE) * PIXELIZATION_SIZE + 0.5 * PIXELIZATION_SIZE;

    float ditherThreshold      = Bayer4(floor(WorldPos));
    float quantizeInverseSteps = 1.0 / float(QUANTIZATION_INTENSITY_STEPS);

    vec3 accumlatedLight = baseColor * AMBIENT;

    LightIndices lightIndices = LightIndicesArray[CameraIndex];
    uint count = lightIndices.LightCount;

    for (uint i = 0u; i < count && i < MAX_LIGHTS_PER_CAMERA; i++) {
        uint lightIndex = lightIndices.Indices[i];
        Light light = Lights[lightIndex];
        if(light.Data.x < ObjectLayer || (light.Data.x > ObjectLayer && int(light.Data.y) == 0)) // layer light check
            continue;

        switch(int(light.Color.w)) { // switch per light type
            case 0:
                accumlatedLight += vec3(1.0, 0.0, 1.0); // invalid light type
                break;
            case 1:
                accumlatedLight += ShadePointLight(light, blockCenter, baseColor, N, V, ditherThreshold, quantizeInverseSteps);
                break;
        }
    }

    FragColor = vec4(accumlatedLight, albedo.a);
}
