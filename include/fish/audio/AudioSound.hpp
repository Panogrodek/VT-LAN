#pragma once
#include "AudioSource.hpp"
#include "AudioDevice.hpp"

namespace fs {
	namespace fs_priv {
		class SoundManager;
	}

	class Sound {
	public:
		Sound() = default;
		Sound(AudioSource* source);
		~Sound();

		Sound(Sound&& other) noexcept;
		Sound& operator=(Sound&& other) noexcept;

		Sound(const Sound&) = delete;
		Sound& operator=(const Sound&) = delete;

		void SetSource(AudioSource* source);

		/*
		@param loops - 0 for one, -1 for infinite
		*/
		void Play(int loops = 0);
		void Stop(int fadeOutFrames = 0);
		void Pause();
		void Resume();

		void SetVolume(float gain);   // 0.0f - 1.0f
		void SetPitch(float ratio);   // 1.0 = normal, 2.0 = double speed
		void SetPosition3D(float x, float y, float z);
		void SetPosition3D(const glm::vec3& pos3D);

		bool IsPlaying() const;
		bool IsValid()   const;

		void Destroy();


		uint16_t GetID() const { return m_ID; };
		const std::string& GetName() const { return m_name; };
	private:
		friend class fs_priv::SoundManager;

		void EnsureTrack();

		float m_gain = 1.0f;
		float m_pitch = 1.0f;

		//ID
		std::string m_name = "";
		uint16_t m_ID = 0;
		
		//owned
		MIX_Track* m_track = nullptr;

		//not owned
		AudioSource* m_source = nullptr;
	};

	namespace fs_priv {
		class SoundManager {
		public:
			//0 for invalid
			uint16_t Add(Sound&& sound, const std::string& name = "");

			Sound* Get(uint16_t id);
			Sound* Get(const std::string& name);

			void Remove(uint16_t id);
			void Remove(const std::string& name);

			void Shutdown();
		private:
			IDAllocator m_allocator;

			std::unordered_map<std::string, Sound*> m_soundsByName;
			std::unordered_map<uint16_t, Sound> m_sounds;
		};
	}

	inline fs_priv::SoundManager soundManager;
}