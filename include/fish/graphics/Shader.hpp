#pragma once

#include "Shader Moudle.hpp"

namespace fs {
	class Shader {
	public:
		Shader()  = default;
		~Shader() = default;

		bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
		bool LoadFromMemory(std::string vertexShader, const std::string& vertexCacheName, std::string fragmentShader, const std::string& fragmentCacheName);

		ShaderModule& GetVertexShader();
		ShaderModule& GetFragmentShader();

	private:
		ShaderModule m_vertexShader;
		ShaderModule m_fragmentShader;
	};
}
