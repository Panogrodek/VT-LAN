#pragma once
#include <sdl3/SDL_audio.h>
#include <opus/opus.h>

#include <deque>
#include <atomic>
#include <mutex>
#include <thread>

#include "networking/ReceiverFlag.hpp"

namespace fs {
	namespace fs_priv {
		/*
		The sampling rate determines how many audio samples per second you record.
		*/
		constexpr uint16_t VOICE_SAMPLE_RATE = 48000; //[Hz]
		
		/*
		from https://wiki.xiph.org/Opus_Recommended_Settings:
		Opus can encode frames of 2.5, 5, 10, 20, 40, or 60 ms
		
		You need to use values listed above for the voice to be encoded correctly if using opus
		*/
		constexpr float VOICE_FRAME_DURATION = 20.0f; //[ms]
		
		/*
		The formula for the amount of samples in one frame
		*/
		constexpr uint16_t VOICE_SAMPLES_IN_FRAME = static_cast<uint16_t>((VOICE_FRAME_DURATION / 1000) * static_cast<float>(VOICE_SAMPLE_RATE));

		/*
		The max amount of stored bytes of audio in SDL stream buffer. Max is about ~80k from sdl
		*/
		constexpr uint32_t MAX_BYTES_IN_STREAM = 11520;

		/*
		The amount of frames that the player needs to be silent in order to not send frames [ms] TODO: needs to be checked
		*/
		constexpr uint8_t IS_SILENT_GRACE_FRAMES = 6;

		struct EncodedFrame {
			uint16_t seq{};
			std::vector<unsigned char> data{};
		};

		class VoiceSendingManager {
		public:
			void Start();
			void Shutdown();

		private:
			void Run();
			void Update();


			void EncodeAndQueueFrame(const float* frame, size_t samples);
			bool IsSilent(const float* frame, size_t samples);

			
			void NetworkingSendData();
		private:
			std::atomic<bool> m_running{ false };
			std::thread m_voiceSendingThread;


			std::atomic<float> m_rmsThreshold = 0.0025f;

			OpusEncoder* m_encoder{};
			std::deque<EncodedFrame> m_encodedFrames;
			std::vector<float> m_capturedAudioData;
			std::size_t m_readPos = 0;

			uint16_t m_nextSeq{};
			uint8_t m_currentSilentFrames = 0;
		};
	}

	inline fs_priv::VoiceSendingManager voiceSendingManager;
}