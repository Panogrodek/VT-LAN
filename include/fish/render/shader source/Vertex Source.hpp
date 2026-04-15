#pragma once

namespace fs {
	namespace fs_priv {
		constexpr const char* FLUSH_VERT = R"(
		#version 460

		layout(location = 0) in  vec2 Position;
		layout(location = 0) out vec2 iTexCoords;
		
		void main() {
		    gl_Position = vec4(Position, 0.0, 1.0);
		    iTexCoords  = (Position + 1) * 0.5;
		}
		)";

		constexpr const char* DEFAULT_VERT = R"(
		#version 460

		layout(location = 0) in vec3 Position;
		layout(location = 1) in vec2 TexCoords;
		layout(location = 2) in vec4 Color;
		
		layout(location = 0) out vec4 iColor;
		layout(location = 1) out vec2 iTexCoords;
		
		layout(set = 1, binding = 0) uniform Uniforms {
		    mat4 Projection;
		    mat4 View;
		    mat4 Model;
		};
		
		void main() {
		    gl_Position = Projection * View * Model * vec4(Position, 1.0);
		    iColor		= Color;
		    iTexCoords  = TexCoords;
		}
		)";

		constexpr const char* DEFAULT_BATCH_VERT = R"(
		#version 460
		
		layout(location = 0) in vec3 Position;
		layout(location = 1) in vec2 TexCoords;
		layout(location = 2) in vec2 NormalTexCoords;
		layout(location = 3) in vec4 Color;
		layout(location = 4) in int  Index;
		
		layout(location = 0) out vec4 iColor;
		layout(location = 1) out vec2 iTexCoords;
		layout(location = 2) out vec2 iNormalTexCoords;
		
		struct UniformData {
		    mat4 Projection;
		    mat4 View;
		    mat4 Model;
		};
		
		layout(set = 0, binding = 0) readonly buffer ModelSSB {
		    UniformData Data[];
		};
		
		void main() {
		    gl_Position = Data[Index].Projection * Data[Index].View * Data[Index].Model * vec4(Position, 1.0);
		    
		    iColor           = Color;
		    iTexCoords       = TexCoords;
		    iNormalTexCoords = NormalTexCoords;
		}
		)";
	}
}
