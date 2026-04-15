#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

#include "utilities/IDAllocator.hpp"
#include "networking/Networking.hpp"

/*

DOCUMENTATION
#documentation for this file is completed and provided on the discord under networking/channel

*/

namespace fs {
	template<typename ChannelData>
	class Channel;

	namespace fs_priv {
		class ChannelBasePtr {
		public:
			using Authentication = std::function<bool(const fs::Connection&)>;
			virtual ~ChannelBasePtr() = default;
			virtual const uint16_t GetChannelID() const final;

		protected:
			virtual void Clear() = 0;
			virtual void Update() = 0;
			virtual void Flush() = 0;

			virtual bool Authenticate(const fs::Connection& con) const = 0;

			uint16_t m_channelID = 0; //0 for invalid
			fs::Authority m_authority = fs::Authority::Invalid;
			std::unordered_map<ReceiverFlag,std::vector<std::string>> m_receivedSerializedData;
		private:
			friend class ChannelContainer;
			friend class Receiver;
		};

		class ChannelContainer {
		public:
			void Init();

			//return the id of the channel created
			uint16_t Add(ChannelBasePtr* channelObject, const std::string& channelName = "");
			
			bool Remove(ChannelBasePtr* channelObject);
			bool RemoveByName(const std::string& channelName);
			bool RemoveByID(uint16_t channelID);

			void Destroy();

			ChannelBasePtr* GetByID(uint16_t channelID);
			ChannelBasePtr* GetByName(const std::string& channelName);

			template<typename T>
			fs::Channel<T>* RetrieveChannel(const std::string& name);
			template<typename T>
			fs::Channel<T>* RetrieveChannel(uint16_t channelId);

			bool IsChannelIDEmpty(uint16_t channelId) const;

			const std::unordered_map<uint16_t, ChannelBasePtr*>& GetAll();

			/*
			NOTE: you have to do something with this id if you retrieve it, otherwise you're just occupying a slot mindlessly
			*/
			uint16_t RetrieveNextUniqueID();
		private:
			template<typename ChannelData>
			friend class fs::Channel;
			friend class Receiver;
			friend class Sender;

			void BindChannelToChannelId(ChannelBasePtr* channel, uint16_t id);

			void ClearChannels();
			void Update();
			void Flush();

			std::unordered_map<uint16_t, ChannelBasePtr*> m_channelIDs;
			std::unordered_map<std::string, ChannelBasePtr*> m_channelNames;

			fs::IDAllocator m_IDAllocator{};
		};
		template<typename T>
		inline fs::Channel<T>* ChannelContainer::RetrieveChannel(const std::string& name)
		{
			auto* channel = GetByName(name);
			if (channel == nullptr) {
				ELOG("Retrieving not existing channel {}!", name);
				return nullptr;
			}

			return dynamic_cast<fs::Channel<T>*>(channel);
		}
		template<typename T>
		inline fs::Channel<T>* ChannelContainer::RetrieveChannel(uint16_t channelId)
		{
			if (IsChannelIDEmpty(channelId)) {
				ELOG("Retrieving from unbound channel {}", channelId);
				return nullptr;
			}

			auto* channel = GetByID(channelId);
			if (channel == nullptr) {
				ELOG("Retrieving not existing channel on id {}!", channelId);
				return nullptr;
			}

			return dynamic_cast<fs::Channel<T>*>(channel);
		}
	}

	inline fs_priv::ChannelContainer channelContainer;

	template<typename ChannelData>
	class Channel : public fs_priv::ChannelBasePtr {
	public:
		using Authentication = std::function<bool(const fs::Connection&)>;
		/*
		@param authority - if set to HOST ONLY then server will ignore channel data send requests from other peers. Otherwise, the channel will
		be left open for everyone else to use
		
		@param receivers - who will receive the data when channel flushes.

		@param channelID - best left empty and assigned by ChannelContainer, unless we want to occupy a specific slot
		*/
		Channel(fs::Authority authority = fs::Authority::Invalid, fs::ReceiverFlag receivers = fs::ReceiverFlag::Invalid, uint16_t channelID = 0);

		const std::unordered_map<ReceiverFlag, std::vector<ChannelData>>& GetData() const;
		std::unordered_map<ReceiverFlag, std::vector<ChannelData>> GetAndRemoveData();

		const std::vector<ChannelData>& GetRequests() const;
		std::vector<ChannelData> GetAndRemoveRequests();


		void AddData(ChannelData data);

		/*
		Closing channel means that it will neither remove, clear nor add data to its containers. It will also not send nor receive data
		*/
		void SetClosed(bool closed = true);
		void BindToChannelId(uint16_t channelID);

		/*
		receivers is a pointer because we might want to modify the recipients at runtime (for example, AllCurrentReceivers might change)
		*/
		void SetReceivers(fs::ReceiverFlag receivers);
		
		/*
		Set steam internal socket flags. For example: send unreliable, send reliable etc.
		*/
		void SetSendFlags(int flags);
		
		virtual void Flush() override;

		/*
		An authentication function, that checks if the client CAN request/send data (NOTE: there are some situtations, where client SHOULD be able to 
		generally add data to the channel, but not at some specific time moment. For example: player wants to send chat message when the game is loading)
		*/
		void SetAuthentication(const Authentication& func);
	private:
		friend class fs_priv::Sender;
		friend class fs_priv::Receiver;

		std::string SerializeData(const ChannelData& data) const;
		std::vector<ChannelData> DeserializeData(const std::string& data) const;

		std::string GetSerializedData() const;

		virtual void Clear() final;
		virtual void Update() final;

		virtual bool Authenticate(const fs::Connection& con) const final;

		void FlushData();
		void FlushRequests();

	private:
		Authentication m_authentication = nullptr;

		int m_flags = k_nSteamNetworkingSend_Reliable;
		bool m_closed = false;
		
		//ReceiverFlag indicates the sender of the original data
		std::unordered_map<ReceiverFlag,std::vector<ChannelData>> m_data;
		std::vector<ChannelData> m_requestedData;
		size_t m_maxDataPacketSize = k_cbMaxSteamNetworkingSocketsMessageSizeSend;
		fs::ReceiverFlag m_receivers = AllFlag;
	};

	template<typename ChannelData>
	inline fs::Channel<ChannelData>::Channel(fs::Authority authority, fs::ReceiverFlag receivers, uint16_t channelId) :
		m_receivers(receivers)
	{
		m_authority = authority;
		m_channelID = channelId;
		if (m_channelID == 0)
			m_channelID = channelContainer.RetrieveNextUniqueID();
		BindToChannelId(m_channelID);
	}


	template<typename ChannelData>
	inline const std::unordered_map<ReceiverFlag, std::vector<ChannelData>>& Channel<ChannelData>::GetData() const
	{
		if (m_closed) {
			static const std::unordered_map<ReceiverFlag, std::vector<ChannelData>> empty;
			return empty;
		}
		return m_data;
	}


	template<typename ChannelData>
	inline std::unordered_map<ReceiverFlag, std::vector<ChannelData>> Channel<ChannelData>::GetAndRemoveData()
	{
		if (m_closed) {
			static const std::unordered_map<ReceiverFlag, std::vector<ChannelData>> empty;
			return empty;
		}
		std::unordered_map<ReceiverFlag, std::vector<ChannelData>> copy = m_data;
		m_data.clear();
		return copy;
	}
	template<typename ChannelData>
	inline const std::vector<ChannelData>& Channel<ChannelData>::GetRequests() const
	{
		if (m_closed) {
			static const std::vector<ChannelData> empty;
			return empty;
		}
		return m_requestedData;
	}
	template<typename ChannelData>
	inline std::vector<ChannelData> Channel<ChannelData>::GetAndRemoveRequests()
	{
		if (m_closed) {
			static const std::vector<ChannelData> empty;
			return empty;
		}
		std::vector<ChannelData> copy = m_requestedData;
		m_requestedData.clear();
		return copy;
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::AddData(ChannelData data)
	{
		if (m_closed)
			return;
		if(networking.Server())
			m_data[MeFlag()].push_back(data);
		else
			m_requestedData.push_back(data);
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::SetClosed(bool closed)
	{
		m_closed = closed;
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::BindToChannelId(uint16_t ID)
	{
		if (ID == 0)
			return;
		if (!channelContainer.IsChannelIDEmpty(ID)) {
			//TODO: fix this on release 
			CLOG("Trying to bind to channel ID {}, but it is already occupied!", ID);
			ALOG("Runtime assertion failed: ID needs to be unique");
			return;
		}

		channelContainer.BindChannelToChannelId(this, ID);
		m_channelID = ID;
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::SetReceivers(fs::ReceiverFlag receivers)
	{
		m_receivers = receivers;
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::SetSendFlags(int flags)
	{
		m_flags = flags;
		if (m_flags & k_nSteamNetworkingSend_Unreliable) {
			m_maxDataPacketSize = k_cbMaxSteamNetworkingSocketsMessageSizeSend / 512;
		}
		if (m_flags & k_nSteamNetworkingSend_Reliable) {
			m_maxDataPacketSize = k_cbMaxSteamNetworkingSocketsMessageSizeSend;
		}
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::Flush()
	{
		if (m_closed)
			return;
		if (m_authority == fs::Authority::Invalid)
			return;
		if (!networking.Server() && m_authority == fs::Authority::HostOnly)
			return;

		if (networking.Server())
			FlushData();
		else
			FlushRequests();
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::SetAuthentication(const Authentication& func)
	{
		m_authentication = func;
	}
	template<typename ChannelData>
	inline std::string Channel<ChannelData>::SerializeData(const ChannelData& data) const
	{
		std::ostringstream oss(std::ios::binary);
		boost::archive::binary_oarchive oa(oss);
		oa << data;
		return oss.str();
	}
	template<typename ChannelData>
	inline std::string Channel<ChannelData>::GetSerializedData() const
	{
		std::ostringstream oss(std::ios::binary);
		boost::archive::binary_oarchive oa(oss);

		for (auto& data : m_data) {
			oa << data;
		}
		return oss.str();
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::Clear()
	{
		if (m_closed)
			return;
		m_data.clear();
		m_requestedData.clear();
		m_receivedSerializedData.clear();
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::Update()
	{
		if (m_closed)
			return;
		for (auto& [sender, receivedData] : m_receivedSerializedData) {
			for (auto& serialized : receivedData) {
				auto data = DeserializeData(serialized);
				m_data[sender].insert(m_data[sender].end(), data.begin(), data.end());
			}
		}
	}
	template<typename ChannelData>
	inline bool Channel<ChannelData>::Authenticate(const fs::Connection& con) const
	{
		if (m_authentication == nullptr)
			return true;
		return m_authentication(con);
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::FlushData()
	{
		if (m_closed)
			return;
		if (m_data.empty())
			return;

		if (m_receivers == fs::ReceiverFlag::Invalid) {
			ELOG("Channel {} has invalid receivers.", m_channelID);
			return;
		}
		auto serialize_size = [](const std::vector<ChannelData>& vec) -> std::size_t {
			std::ostringstream oss(std::ios::binary);
			{
				boost::archive::binary_oarchive oa(oss);
				oa << vec;
			} 
			return oss.str().size();
		};

		std::vector<ChannelData> batch;
		uint16_t countInBatch = 0;

		auto flush_batch = [&](std::vector<ChannelData>& toSend, uint16_t count) {
			if (toSend.empty()) return;
			std::ostringstream oss(std::ios::binary);
			{
				boost::archive::binary_oarchive oa(oss);
				oa << toSend;
			}
			const std::string payload = oss.str();
			fs::sender.SendChannelSerializedData(m_receivers, payload, m_channelID, count, m_flags);
			toSend.clear();
		};

		for (auto& [receiver, allData] : m_data) {
			for (const auto& data : allData) {
				std::vector<ChannelData> candidate = batch;
				candidate.push_back(data);

				const std::size_t candidateSize = serialize_size(candidate);

				if (candidateSize > m_maxDataPacketSize) {
					// If even a single item can't fit in an empty batch, bail
					if (batch.empty()) {
						ELOG("Serialized size of a single item for channel {} exceeds packet limit ({} bytes).",
							m_channelID, m_maxDataPacketSize);
						return;
					}
					// Flush current batch, then start a new one with this item
					flush_batch(batch, countInBatch);
					countInBatch = 0;

					// Re-check item alone
					candidate.clear();
					candidate.push_back(data);
					if (serialize_size(candidate) > m_maxDataPacketSize) {
						ELOG("Serialized size of a single item for channel {} exceeds packet limit ({} bytes).",
							m_channelID, m_maxDataPacketSize);
						return;
					}

					batch.push_back(data);
					++countInBatch;
				}
				else {
					batch.push_back(data);
					++countInBatch;
				}
			}
		}

		// Flush remainder
		flush_batch(batch, countInBatch);

		// m_data.clear();
	}
	template<typename ChannelData>
	inline void Channel<ChannelData>::FlushRequests()
	{
		if (m_closed)
			return;
		if (m_requestedData.size() <= 0)
			return;
		if (m_receivers == fs::ReceiverFlag::Invalid) {
			ELOG("Channel {} has invalid receivers.", m_channelID);
			return;
		}
		if (m_authority == fs::Authority::HostOnly)
			return;

		auto serialize_size = [](const std::vector<ChannelData>& vec) -> std::size_t {
			std::ostringstream oss(std::ios::binary);
			{
				boost::archive::binary_oarchive oa(oss);
				oa << vec;
			}
			return oss.str().size();
			};

		std::vector<ChannelData> batch;
		batch.reserve(m_requestedData.size());
		uint16_t countInBatch = 0;

		auto flush_batch = [&](std::vector<ChannelData>& toSend, uint16_t count) {
			if (toSend.empty()) return;
			std::ostringstream oss(std::ios::binary);
			{
				boost::archive::binary_oarchive oa(oss);
				oa << toSend;
			}
			const std::string payload = oss.str();
			fs::sender.SendChannelSerializedDataRequest(m_receivers, payload, m_channelID, count, m_flags);
			toSend.clear();
			};

		for (const auto& data : m_requestedData) {
			std::vector<ChannelData> candidate = batch;
			candidate.push_back(data);

			const std::size_t candidateSize = serialize_size(candidate);

			if (candidateSize > m_maxDataPacketSize) {
				// If even a single item can't fit in an empty batch, bail
				if (batch.empty()) {
					ELOG("Serialized size of a single item for channel {} exceeds packet limit ({} bytes).",
						m_channelID, m_maxDataPacketSize);
					return;
				}
				// Flush current batch, then start a new one with this item
				flush_batch(batch, countInBatch);
				countInBatch = 0;

				// Re-check item alone
				candidate.clear();
				candidate.push_back(data);
				if (serialize_size(candidate) > m_maxDataPacketSize) {
					ELOG("Serialized size of a single item for channel {} exceeds packet limit ({} bytes).",
						m_channelID, m_maxDataPacketSize);
					return;
				}

				batch.push_back(data);
				++countInBatch;
			}
			else {
				batch.push_back(data);
				++countInBatch;
			}
		}

		// Flush remainder
		flush_batch(batch, countInBatch);

		// m_requestedData.clear();
	}
	template<typename ChannelData>
	inline std::vector<ChannelData> Channel<ChannelData>::DeserializeData(const std::string& data) const
	{
		std::stringbuf buf(data);
		std::istream is(&buf);
		boost::archive::binary_iarchive ia(is);

		std::vector<ChannelData> out;
		ia >> out;
		return out;
	}
}