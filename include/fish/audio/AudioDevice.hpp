#pragma once
#include <sdl3/SDL_audio.h>
#include <sdl3_mixer/SDL_mixer.h>
#include <utilities/IDAllocator.hpp>
#include <set>
#include <memory>
#include <shared_mutex>
/*
Important note:
Audio device is both and INPUT and OUTPUT device. It can be used to access hardware speakers and microphone.
Because of this, you can push and pull data from and to the device
*/

namespace fs {
	namespace fs_priv {
		class AudioDeviceManager;
	}

	enum class AudioDeviceKind : uint8_t {
		Invalid = 0,
		Playback,
		Recording
	};

	struct AudioDevice {
	public:
		std::shared_ptr<MIX_Mixer> GetPlaybackHandle() const;
		std::shared_ptr<SDL_AudioStream> GetRecordingHandle() const;

		AudioDeviceKind GetDeviceKind() const;
	private:
		friend class fs_priv::AudioDeviceManager;

		AudioDeviceKind m_kind = AudioDeviceKind::Invalid;

		//The device ID provided by SDL
		SDL_AudioDeviceID m_deviceID = 0; //Zero is used to signify an invalid/null device. /source: sdl wiki
		SDL_AudioSpec m_audioSpecs{};
		std::string m_name{};

		std::shared_ptr<SDL_AudioStream> m_audioStream;
		std::shared_ptr<MIX_Mixer> m_mixer;
	};




	namespace fs_priv {
		class AudioDeviceManager {
		public:
			using AudioDeviceName = std::string;

			std::shared_ptr<const fs::AudioDevice> GetPlaybackDevice() const;
			std::shared_ptr<const fs::AudioDevice> GetRecordingDevice() const;

			/*
			Tell SDL to update our devices list according to what the OS lists
			*/
			void UpdateAvailableDevicesList();

			std::unordered_map<SDL_AudioDeviceID, std::string> GetAvailablePlaybackDevicesList() const;
			std::unordered_map<SDL_AudioDeviceID, std::string> GetAvailableRecordingDevicesList() const;

			/*
			* This function overrides any previously opened device
			@param kind - can be recording or playback according to our needs
			@param ID - SDL api provided ID for the device. Check SDLList accessor functions provided for this manager
			@param specs - wanted audio specs for this device
			*/
			bool OpenNewDevice(fs::AudioDeviceKind kind, SDL_AudioDeviceID ID, const SDL_AudioSpec* specs = nullptr);
			
			bool OpenDefaultDevice(fs::AudioDeviceKind kind, const SDL_AudioSpec* specs = nullptr);


			void CloseDevice(fs::AudioDeviceKind kind);
			void Shutdown();
		private:
			void ListAvailablePlaybackDevices();
			void ListAvailableRecordingDevices();
		private:
			mutable std::shared_mutex m_audioDeviceMtx;

			std::unordered_map<SDL_AudioDeviceID, AudioDeviceName> m_availablePlaybackDevices;
			std::unordered_map<SDL_AudioDeviceID, AudioDeviceName> m_availableRecordingDevices;

			std::shared_ptr<fs::AudioDevice> m_currentRecordingDevice;
			std::shared_ptr<fs::AudioDevice> m_currentPlaybackDevice;
		};
	}

	inline fs_priv::AudioDeviceManager audioDeviceManager;
}

