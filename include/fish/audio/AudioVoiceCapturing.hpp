#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <algorithm>
#include <utility> 

namespace fs {
	namespace fs_priv {
		class VoiceSendingManager;

		/*
		The amount of stored audio frames. The formula for the amount of "seconds of voice data" that we can store in a buffer is
			SECONDS_OF_VOICE [ms] = AUDIO_DATA_BUFFER_SIZE / VOICE_SAMPLE_RATE
		*/
		constexpr uint16_t AUDIO_DATA_BUFFER_SIZE = 4096;
	}
	
	struct VoiceBuffer
	{
		std::array<float, fs_priv::AUDIO_DATA_BUFFER_SIZE> samples{};
		std::size_t count = 0;
	};

	namespace fs_priv {
		class VoiceCaptureManager {
		public:
			void Start();
			void Shutdown();

			void SwapUserVoiceBuffer(VoiceBuffer& out);
		private:
			friend class VoiceSendingManager;
			
			void SwapSendVoiceBuffer(VoiceBuffer& out);


			void Run();

			void Update();
		private:
			std::atomic<bool> m_running{ false };
			std::thread m_voiceCaptureThread;

			std::array<float, AUDIO_DATA_BUFFER_SIZE> m_capturedAudioTempBuffer;

			//voice buffer to be used by app (for example, in singleplayer to detect the voice level)
			VoiceBuffer m_userVoiceBufferTemp; 
			VoiceBuffer m_userVoiceBuffer; 
			std::mutex m_userMtx;


			//voice buffer of VoiceSendingManager
			VoiceBuffer m_sendVoiceBufferTemp;
			VoiceBuffer m_sendVoiceBuffer;
			std::mutex m_sendMtx;
		};
	}
	inline fs_priv::VoiceCaptureManager voiceCaptureManager;
}