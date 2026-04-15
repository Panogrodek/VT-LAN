#pragma once
#include <mutex>
#include <vector>
#define VOICE_CHAT_TEST 1

namespace fs {
	namespace fs_priv {
		//fd
		class VoiceReceivingManager;

		/*
		defines the max amount of ticks that can be shown on implot whilst testing. One tick is equal to 0.001 [s]		
		*/
		constexpr int MAX_TICKS_SHOWN = 3000;

		template<typename T>
		struct VoiceChatTestDataContainer
		{
			T buffer[MAX_TICKS_SHOWN]{};
			size_t head{0};
			size_t size{0};

			void Push(const T& value) {
				buffer[head] = value;
				head = (head + 1) % MAX_TICKS_SHOWN;
				if (size < MAX_TICKS_SHOWN)
					size++;
			}

			void OverwriteLatest(const T& value) {
				if (size == 0) {
					Push(value);
					return;
				}
				const size_t latestIndex = (head + MAX_TICKS_SHOWN - 1) % MAX_TICKS_SHOWN;
				buffer[latestIndex] = value;
			}
		};

		class VoiceChatTest {
		public:
			void SnapshotReceivedStateData(std::vector<uint32_t>& outX, std::vector<uint32_t>& outY);
			void SnapshotReceivedBytesData(std::vector<uint32_t>& outX, std::vector<uint32_t>& outY);
		private:
			friend class VoiceReceivingManager;
			void StepReceived();

			void PushReceivedStateSample(uint32_t sample);
			void PushReceivedBytesSample(uint32_t sample);
		private:
			std::mutex m_receivedDataMutex;

			uint32_t m_receivedTick = 0;

			//stored -> 0,1,2 where 0 - none, 1 - normal, 2 - PLC, consult: VoiceReceivingManager::Play
			VoiceChatTestDataContainer<uint32_t> m_receivedStateData;
			VoiceChatTestDataContainer<uint32_t> m_receivedBytesData;


		};
	}

	inline fs_priv::VoiceChatTest voiceChatTest;
}