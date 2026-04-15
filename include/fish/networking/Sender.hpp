#pragma once
#include "Connection.hpp"

/*

DOCUMENTATION
#documentation for this file is completed and provided on the discord under networking/Sender

*/

namespace fs {
	namespace fs_priv {
		//Ideally, you shouldn't have to deal with it as a user
		enum class NetworkingHeader : uint8_t {
			Invalid = 0,
			SendChannelData = 1,
			RPCCall = 2,

			SendChannelDataRequest = 101,
			RPCCallRequest = 102,

			VoicePacket = 201,
		};

		class Sender {
		public:
			//---------------------------------------------------------------------
			// Server side send commands
			//---------------------------------------------------------------------
			template<typename Data>
			void SendChannelData(fs::ReceiverFlag receivers, Data data, const std::string& channel, uint16_t count) const;
			template<typename Data>
			void SendChannelData(fs::ReceiverFlag receivers, Data data, uint16_t channel, uint16_t count) const;

			void SendChannelSerializedData(fs::ReceiverFlag receivers, const std::string& serializedData, const std::string& channelName, uint16_t count, int flags = k_nSteamNetworkingSend_Reliable) const;
			void SendChannelSerializedData(fs::ReceiverFlag receivers, const std::string& serializedData, uint16_t channelID, uint16_t count, int flags = k_nSteamNetworkingSend_Reliable) const;

			template<typename ...Args>
			void SendRPC(fs::ReceiverFlag receivers, const fs::RPC* rpc, Args&&... args) const;

			void ForwardRPCRequest(fs::ReceiverFlag receivers, const fs::RPC* rpc, const std::string& serializedArgs) const;

			//---------------------------------------------------------------------
			// Client side send commands (requests)
			//---------------------------------------------------------------------
			void SendChannelSerializedDataRequest(fs::ReceiverFlag receivers, const std::string& serializedData, const std::string& channel, uint16_t count, int flags = k_nSteamNetworkingSend_Reliable) const;
			void SendChannelSerializedDataRequest(fs::ReceiverFlag receivers, const std::string& serializedData, uint16_t channel, uint16_t count, int flags = k_nSteamNetworkingSend_Reliable) const;
			
			template<typename ...Args>
			void SendRPCRequest(fs::ReceiverFlag receivers, const fs::RPC* rpc, Args&&... args) const;

		private:
			friend class VoiceSendingManager;
			//---------------------------------------------------------------------
			// Both sides viable
			//---------------------------------------------------------------------

			//NON THREAD SAFE
			void SendVoicePacket(fs::ReceiverFlag receivers, std::vector<unsigned char>& data, uint16_t sequenceID) const;
		private:
			friend class NetworkingManager;


			void Flush(); //sends all the messages stored
		private:

			struct MessageDataType {
				HSteamNetConnection receiver = k_HSteamNetConnection_Invalid;
				std::vector<char> data{};
				size_t dataSize = 0;
				int flags = k_nSteamNetworkingSend_Reliable;


				MessageDataType() = default;

				MessageDataType(HSteamNetConnection recv, std::vector<char> _data, size_t size, int _flags = k_nSteamNetworkingSend_Reliable)
					: receiver(recv), data(_data), dataSize(size), flags(_flags) {}
			};

			mutable std::vector<MessageDataType> m_messagesToSend; //server side
			mutable std::vector<MessageDataType> m_requestsToSend; //client side
		};

		template<typename Data>
		inline void Sender::SendChannelData(fs::ReceiverFlag receivers, Data data, const std::string& channel, uint16_t count) const
		{
			auto* Chan = fs::channelContainer.Get(channel);
			if (Chan != nullptr) {
				SendChannelData(receivers, data, Chan->GetChannelId(), count);
				return;
			}
			ELOG("Trying to use non existing channel {} to send data", channel);
		}

		template<typename Data>
		inline void Sender::SendChannelData(fs::ReceiverFlag receivers, Data data, uint16_t channel, uint16_t count) const
		{
			std::string stringData = fs::channelContainer.RetrieveChannel<Data>(channel)->SerializeData(data);
			SendChannelSerializedData(receivers, stringData, channel, count);
		}

		template<typename ...Args>
		inline void Sender::SendRPC(fs::ReceiverFlag receivers, const fs::RPC* rpc, Args && ...args) const
		{
			if (!fs::networking.Server()) {
				ELOG("[CLIENT] Client trying to send data as server. Use request instead.");
				return;
			}

			if (rpc == nullptr) {
				ELOG("[HOST] Cannot send rpc, rpc is corrupted (rpc is nullptr)!");
				return;
			}
			uint16_t id = rpc->GetId();
			if (id == UINT16_MAX || id == 0) {
				ELOG("[HOST] Cannot send rpc, rpc is corrupted (rpc id)!");
				return;
			}
			if (receivers == fs::ReceiverFlag::Invalid /*|| !HasAllFlags(fs::AllCurrentReceivers,receivers)*/) {
				ELOG("[HOST] Cannot send rpc, receivers are invalid");
				return;
			}

			std::string data = rpc->SerializeData(std::forward<Args>(args)...);
			if (data.size() <= 0) {
				ELOG("[HOST] Cannot send rpc, dataSize is incorrect!");
				return;
			}

			std::vector<char> buffer;
			//the networking header + the rpc id + data size
			buffer.resize(sizeof(uint8_t) + sizeof(uint16_t) + data.size());

			NetworkingHeader header = NetworkingHeader::RPCCall;

			std::memcpy(buffer.data(), &header, sizeof(uint8_t));

			std::memcpy(buffer.data() + sizeof(uint8_t), &id, sizeof(uint16_t));

			std::memcpy(buffer.data() + sizeof(uint8_t) + sizeof(uint16_t), data.data(), data.size());

			for (auto& con : GetAllReceiversInFlag(receivers)) {
				if (con == fs::MeFlag()) {
					std::vector<char> copy = buffer;
					std::memmove(copy.data(), copy.data() + sizeof(uint8_t), copy.size() - sizeof(uint8_t)); //because we skip networking header
					copy.resize(copy.size() - sizeof(uint8_t));
					ReceiverMessage message;
					message.connectionID = connections.GetByGameID(ReceiverFlagToGameID(con)).GetHSteamNetConnectionID();
					message.header = NetworkingHeader::RPCCall;
					message.data = std::move(copy);
					message.toSelf = true;
					receiver.m_messages.push_back(std::move(message));
					continue;
				}
				auto connection = fs::connections.GetByGameID(ReceiverFlagToGameID(con));
				if (!connection.IsValid())
					continue;

				std::vector<char> copy = buffer;
				m_messagesToSend.emplace_back(connection.GetHSteamNetConnectionID(), copy, copy.size());
			}
		}
		template<typename ...Args>
		inline void Sender::SendRPCRequest(fs::ReceiverFlag receivers, const fs::RPC* rpc, Args && ...args) const
		{
			if (fs::networking.Server()) {
				ELOG("[HOST] Server trying to send data as client. Use send instead.");
				return;
			}
			if (rpc == nullptr) {
				ELOG("[CLIENT] Cannot send rpc, rpc is corrupted (rpc is nullptr)!");
				return;
			}
			uint16_t id = rpc->GetId();
			if (id == UINT16_MAX || id == 0) {
				ELOG("[CLIENT] Cannot send rpc, rpc is corrupted (rpc id)!");
				return;
			}
			if (receivers == fs::ReceiverFlag::Invalid /*|| !HasAllFlags(fs::AllCurrentReceivers, receivers)*/) {
				ELOG("[CLIENT] Cannot send rpc, receivers are invalid");
				return;
			}

			std::string data = rpc->SerializeData(std::forward<Args>(args)...);
			if (data.size() <= 0) {
				ELOG("[CLIENT] Cannot send rpc, dataSize is incorrect!");
				return;
			}

			std::vector<char> buffer;
			//the networking header + the rpc id + data size
			buffer.resize(sizeof(uint8_t) + sizeof(fs::ReceiverFlag) + sizeof(uint16_t) + data.size());

			NetworkingHeader header = NetworkingHeader::RPCCallRequest;

			std::memcpy(buffer.data(), &header, sizeof(uint8_t));

			std::memcpy(buffer.data() + sizeof(uint8_t), &receivers, sizeof(fs::ReceiverFlag));

			std::memcpy(buffer.data() + sizeof(uint8_t) + sizeof(fs::ReceiverFlag), &id, sizeof(uint16_t));

			std::memcpy(buffer.data() + sizeof(uint8_t) + sizeof(fs::ReceiverFlag) + sizeof(uint16_t), data.c_str(), data.size());

			m_requestsToSend.emplace_back(fs::connections.GetHost().GetHSteamNetConnectionID(), buffer, buffer.size());
		}
	}

	inline fs_priv::Sender sender;
}