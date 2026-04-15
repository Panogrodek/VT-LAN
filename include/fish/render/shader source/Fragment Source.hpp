#pragma once

namespace fs {
	namespace fs_priv {
		constexpr const char* FLUSH_FRAG = R"(
		#version 460
		
		layout(location = 0) in vec2 TexCoords;
		
		layout(location = 0) out vec4 FragColor;
		
		layout(set = 2, binding = 0) uniform sampler2D Texture;
		
		void main() {
		    FragColor = vec4(texture(Texture, TexCoords).rgb, 1.0);
		}
		)";

		constexpr const char* DEFAULT_FRAG = R"(
		#version 460

		layout(location = 0) in vec4 Color;
		layout(location = 1) in vec2 TexCoords;
		
		layout(location = 0) out vec4 FragColor;
		
		layout(set = 2, binding = 0) uniform sampler2D Texture;
		layout(set = 2, binding = 1) uniform sampler2D NormalTexture;
		
		void main() {
		    FragColor = texture(Texture, TexCoords) * Color;
		}
		)";

		constexpr const char* DEFAULT_BATCH_FRAG = R"(
		#version 460
		
		layout(location = 0) in vec4 Color;
		layout(location = 1) in vec2 TexCoords;
		layout(location = 2) in vec2 NormalTexCoords;
		
		layout(location = 0) out vec4 FragColor;
		
		layout(set = 2, binding = 0) uniform sampler2D Texture;
		layout(set = 2, binding = 1) uniform sampler2D NormalTexture;
		
		void main() {
		    FragColor = texture(Texture, TexCoords) * Color;
		}
		)";
	}
}
