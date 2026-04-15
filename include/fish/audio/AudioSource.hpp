#pragma once
#include <sdl3/SDL_audio.h>
#include <sdl3_mixer/SDL_mixer.h>

namespace fs {
	namespace fs_priv {
		class AudioSourceManager;
	}
	class AudioSource {
	public:
		AudioSource() = default;
		~AudioSource();

		AudioSource(const AudioSource& other) = delete;
		AudioSource& operator=(const AudioSource& other) = delete;

		AudioSource(AudioSource&& other) noexcept;
		AudioSource& operator=(AudioSource&& other) noexcept;


		bool LoadFromFile(const std::string& path, bool precode = false, MIX_Mixer* prefferedMixer = nullptr);

		void ClearData();

		bool IsLoaded() const;

		bool GetInitialFormat(SDL_AudioSpec& out) const;
		Sint64 GetDurationFrames() const;
		float  GetDurationSeconds() const;

		MIX_Audio* GetAudio() const { return m_audio; }

	private:
		MIX_Audio* m_audio = nullptr;
		friend class fs_priv::AudioSourceManager;
	};
}