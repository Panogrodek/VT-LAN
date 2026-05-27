#pragma once
#include "networking/Receiver.hpp"

#include "audio/AudioVoiceSending.hpp"
#include <sdl3_mixer/SDL_mixer.h>

#include "system/Clock.hpp"

#include <glm/vec3.hpp>
#include <thread>
#include <map>

namespace fs {
	class Runtime;

	namespace fs_priv {
		class VoiceReceivingManager;
		class Receiver;
	}

	struct PlayerVoiceSettings {
		bool muted = false;

		// Volume gain
		// IMPORTANT! This is shown as a percentage, so 1.0f is 100%. Be cautious when setting this to higher than 2.0 values
		float gain = 1.0f;

		//positional
		bool positional = false;
		glm::vec3 pos{};
		float maxDistance = 25.0f;  // cutoff distance for volume falloff

		//TODO: bool isSpeaking = false;   // last frame had audible samples

		bool valid = false;
	};

	struct PlayerVoiceData {
	public:
		bool Valid() const { return m_initialized; }
	private:
		friend class fs_priv::VoiceReceivingManager;
		friend class fs_priv::Receiver;
		bool m_initialized = false;

		OpusDecoder* m_decoder{};
		SDL_AudioStream* m_stream{};
		MIX_Track* m_track{};

		uint16_t m_expectedSeq = 0;
		std::map<uint16_t, fs_priv::EncodedFrame> m_pendingVoicePackets{};
	};

	namespace fs_priv {

		/*
		The amount of time the thread will sleep after playing one Run iteration
		*/
		constexpr double VOICE_THREAD_SLEEP = 0.001; //[s]


		class VoiceReceivingManager {
		public:
			void Start();
			void Shutdown();

			/*
			While you can't control when the audio is being played (due to opus frame needing to be in seperate timestep)
			you can control other audio details. 
			*/

			void SetPlayerVoiceSettings(uint8_t gameID, const PlayerVoiceSettings& settings);
			PlayerVoiceSettings GetPlayerVoiceSettings(uint8_t gameID);
		private:
			friend class Receiver;
			friend class Runtime;
			
			void Run();
			/*
			Plays all the data stored in memory of the player. It doesnt override previous data, it appends it at the end
			Note: uses GetPlaybackDevice from AudioDeviceManager
			*/
			void Play(uint8_t playerID);
			void PlayAll();

			void PollAndProcessMessages();


			void PlayReceivedVoice(uint8_t playerID, opus_int16* buffer, int sampleCount);
			bool InitPlayer(uint8_t playerID);
			void ShutdownPlayer(uint8_t playerID);

			void ForwardFrameData();

			std::atomic<bool> m_running{ false };
			std::thread m_voiceReceivingThread;
			std::mutex m_playerVoiceSettingsMtx;

			std::vector<ReceiverMessage> m_incomingMessages;
			//key - connection GameID
			std::unordered_map<uint8_t, PlayerVoiceData> m_playerData;
			std::unordered_map<uint8_t, PlayerVoiceSettings> m_settings;

			Clock m_playbackClock;
			float s_voiceAccumulator{};

			//server buffer
			std::unordered_map<uint8_t, std::vector<EncodedFrame>> m_clientForwardData;
		};
	}

	inline fs_priv::VoiceReceivingManager voiceReceivingManager;
}