#pragma once
#include "Receiver.hpp"
#include "system/Clock.hpp"
#include <mutex>
#include <vector>

namespace fs {
	namespace fs_priv {
		//list of all special multithreaded cases
		class VoiceReceivingManager;
	}
	namespace fs_priv {
		/*
		The max amount of messages that can be polled from a connection in a single iteration of ReceivingThread::Poll
		*/
		constexpr int MAX_MESSAGES = 32;

		/*
		The amount of time the polling thread will sleep after receiving messages. This value should be non zero as to not starve other threads
		*/
		constexpr double POLLING_TIMESTEP = 0.001; //[s]
		

		class ReceivingThread {
		public:
			void Start();
			void Shutdown();
			/*
			Swaps m_polledMessages buffer with input buffer for easy data extraction
			*/
			void SwapMessagesBuffer(std::vector<ReceiverMessage>& out);
		private:
			friend class fs_priv::VoiceReceivingManager;

			//special cases:
			void SwapVoiceBuffer(std::vector<ReceiverMessage>& out);
		private:
			void Run();
			void Poll();

			void StoreMessage(ReceiverMessage&& message);
			void WriteToBuffers();

			//main messsage buffer
			std::mutex m_polledMtx;
			std::vector<ReceiverMessage> m_tempPolledMessages;
			std::vector<ReceiverMessage> m_polledMessages;

			//special case - voice
			std::mutex m_voiceMtx;
			std::vector<ReceiverMessage> m_tempPolledVoicePackets;
			std::vector<ReceiverMessage> m_polledVoicePackets;



			std::atomic<bool> m_running{ false };
			std::thread m_pollingThread;
		};

		inline ReceivingThread receivingThread;
	}

}