#pragma once
#include "Sender.hpp"

/*

DOCUMENTATION
#documentation for this file is completed and provided on the discord under networking/Receiver

*/

namespace fs {
	namespace fs_priv {
		struct ReceiverMessage {
			NetworkingHeader header;
			HSteamNetConnection connectionID = k_HSteamNetConnection_Invalid;
			std::vector<char> data{};

			bool toSelf = false; //local flag not send through net, only to be used for self RPC/Channel calls
		};

		class Receiver {
		private:
			void ReceiveAll();
			void FlushAll();

			bool ReceiveHeaderSort(const NetworkingHeader& header, const fs::Connection& con, char* dataptr, size_t size);

			bool ReceiveChannelData(const fs::Connection& con, char* dataptr, size_t size);
			bool ReceiveRPC(const fs::Connection& con, char* dataptr, size_t size);
			bool ReceiveChannelDataRequest(const fs::Connection& con, char* dataptr, size_t size);
			bool ReceiveRPCRequest(const fs::Connection& con, char* dataptr, size_t size);
		private:
			std::vector<ReceiverMessage> m_messages;
			friend class NetworkingManager;
			friend class Sender;
		};
	}
	inline fs_priv::Receiver receiver;
}
