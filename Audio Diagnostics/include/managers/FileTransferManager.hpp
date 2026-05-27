#pragma once
#include <string>
#include <chrono>
#include <deque>
#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <atomic>

// ReceiverFlag is a plain uint16_t enum class with no Steam dependencies
#include <networking/ReceiverFlag.hpp>

class FileTransferManager {
public:
	// Steam GNS reliable send buffer is ~512 KB per connection.
	// 128 KB chunks: 4 chunks = 512 KB, which is right at the limit.
	// We keep this size but rate-limit HOW MANY are sent per tick window.
	static constexpr size_t CHUNK_SIZE      = 128 * 1024;
	// Max chunks dispatched to the Steam send queue per rate-limit window (~16ms).
	// 3 × 128 KB = 384 KB per window — leaves headroom below the 512 KB buffer limit.
	static constexpr int    CHUNKS_PER_TICK = 3;

	struct ReceivedFile {
		uint8_t     senderID;
		std::string filename;
		std::string savedPath;
		uint64_t    totalSize;
	};

	struct TransferProgress {
		uint32_t    transferID;
		uint8_t     senderID;
		std::string filename;
		uint32_t    chunksReceived;
		uint32_t    totalChunks;
	};

	// Read file from disk, fragment it, and enqueue all fragments.
	// Fragments are dispatched to the network gradually via Tick().
	void Send(uint8_t selfID, const std::string& filepath);

	// Drain the pending send queue — call once per frame from PollNetworkMessages().
	// Rate-limited to CHUNKS_PER_TICK per 16ms window so we never overflow Steam's
	// ~512 KB reliable-send buffer.
	void Tick();

	// Returns (and clears) completed incoming transfers since the last call.
	std::vector<ReceivedFile> PollCompleted();

	// Returns (and clears) latest progress snapshots since the last call.
	std::vector<TransferProgress> PollProgress();

	// Called from RPC handlers (main-thread networking receive pass).
	void OnTransferStart(uint32_t transferID, uint8_t senderID,
	                     const std::string& filename, uint64_t totalSize, uint32_t totalChunks);
	void OnChunkReceived(uint32_t transferID, uint32_t chunkIndex,
	                     const std::vector<uint8_t>& data);

private:
	// One outgoing fragment waiting to be dispatched to Steam GNS.
	struct PendingChunk {
		uint32_t            transferID;
		uint32_t            chunkIndex;
		std::vector<uint8_t> data;
		bool                isServer; // use SendRPC (true) or SendRPCRequest (false)
		fs::ReceiverFlag    targets;
	};

	struct IncomingTransfer {
		uint8_t     senderID;
		std::string filename;
		uint64_t    totalSize   = 0;
		uint32_t    totalChunks = 0;
		std::map<uint32_t, std::vector<uint8_t>> chunks;
	};

	// Must be called with m_mutex already held.
	bool TryAssembleLocked(uint32_t transferID);

	// Pending outgoing chunks (main-thread only, no mutex needed).
	std::deque<PendingChunk> m_pendingChunks;
	std::chrono::steady_clock::time_point m_lastTickTime{};

	// Incoming transfer state and notification queues (accessed under m_mutex).
	std::mutex m_mutex;
	std::unordered_map<uint32_t, IncomingTransfer> m_incoming;
	std::deque<ReceivedFile>     m_completed;
	std::deque<TransferProgress> m_progress;

	std::atomic<uint32_t> m_nextTransferID{ 1 };
};

extern FileTransferManager fileTransferManager;
