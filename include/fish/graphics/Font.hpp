#pragma once

#include <string>

struct TTF_Font;

namespace fs {
	class Font {
	public:
		Font()  = default;
		~Font();

		Font(const Font& other)  = delete;
		Font(const Font&& other) = delete;

		Font& operator=(const Font& other)  = delete;
		Font& operator=(const Font&& other) = delete;
		
		bool LoadFromFile(const std::string& file, uint32_t size);

		TTF_Font* GetHandle();

		uint32_t GetSize() const;

	private:
		TTF_Font* m_handle = nullptr;
		uint32_t  m_size = 32;

		void Destroy();
	};
}
