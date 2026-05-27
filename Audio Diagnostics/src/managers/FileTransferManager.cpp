#include "managers/FileTransferManager.hpp"
#include <networking/Networking.hpp>
#include <networking/RPC.hpp>

#include <fstream>
#include <filesystem>

FileTransferManager fileTransferManager;

static fs::RPCRegistrar reg_ft_start(
	"FileTransferStart",
	fs::Authority::All,
	[](uint32_t transferID, uint8_t senderID, std::string filename,
	   uint64_t totalSize, uint32_t totalChunks) {
		auto self = fs::connections.GetSelf();
		if (self.IsValid() && self.GetGameID() == senderID)
			return;
		fileTransferManager.OnTransferStart(transferID, senderID, filename, totalSize, totalChunks);
	}
);

static fs::RPCRegistrar reg_ft_chunk(
	"FileTransferChunk",
	fs::Authority::All,
	[](uint32_t transferID, uint32_t chunkIndex, std::vector<uint8_t> data) {
		fileTransferManager.OnChunkReceived(transferID, chunkIndex, data);
	}
);

// ---------------------------------------------------------------------------

void FileTransferManager::Send(uint8_t selfID, const std::string& filepath) {
	if (!fs::networking.ActiveSession())
		return;

	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) return;

	const uint64_t fileSize = static_cast<uint64_t>(file.tellg());
	if (fileSize == 0) return;
	file.seekg(0);

	std::vector<uint8_t> fileData(static_cast<size_t>(fileSize));
	if (!file.read(reinterpret_cast<char*>(fileData.data()),
	               static_cast<std::streamsize>(fileSize)))
		return;
	file.close();

	const uint32_t totalChunks =
		static_cast<uint32_t>((fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE);
	const uint32_t transferID  = m_nextTransferID.fetch_add(1, std::memory_order_relaxed);
	const std::string filename = std::filesystem::path(filepath).filename().string();

	const fs::RPC* rpcStart = fs::getRPCMap().GetRPC("FileTransferStart");
	if (!rpcStart) return;
	// rpcChunk existence is checked in Tick() - no point aborting the start if it were missing.

	const bool isServer         = fs::networking.Server();
	const fs::ReceiverFlag targets = fs::AllCurrentReceivers();

	// Send FileTransferStart immediately - it's tiny and must arrive before any chunk.
	if (isServer)
		fs::sender.SendRPC(targets, rpcStart, transferID, selfID, filename, fileSize, totalChunks);
	else
		fs::sender.SendRPCRequest(targets, rpcStart, transferID, selfID, filename, fileSize, totalChunks);

	// Enqueue all chunks for rate-limited delivery via Tick().
	// This prevents overwhelming Steam GNS's ~512 KB per-connection send buffer.
	for (uint32_t i = 0; i < totalChunks; ++i) {
		const size_t offset = static_cast<size_t>(i) * CHUNK_SIZE;
		const size_t size   = std::min(CHUNK_SIZE, static_cast<size_t>(fileSize) - offset);
		PendingChunk pc;
		pc.transferID = transferID;
		pc.chunkIndex = i;
		pc.data       = std::vector<uint8_t>(fileData.begin() + offset,
		                                     fileData.begin() + offset + size);
		pc.isServer   = isServer;
		pc.targets    = targets;
		m_pendingChunks.push_back(std::move(pc));
	}
}

void FileTransferManager::Tick() {
	if (m_pendingChunks.empty()) return;

	// Rate-limit: dispatch at most CHUNKS_PER_TICK chunks per 16ms window.
	// This keeps the total data handed to Steam GNS in any one flush well below
	// the ~512 KB per-connection reliable send buffer.
	auto now = std::chrono::steady_clock::now();
	auto msSinceLastTick = std::chrono::duration_cast<std::chrono::milliseconds>(
		now - m_lastTickTime).count();

	// On the very first call m_lastTickTime is the epoch - treat it as "ready".
	const bool firstTick = (m_lastTickTime == std::chrono::steady_clock::time_point{});
	if (!firstTick && msSinceLastTick < 15)
		return;

	m_lastTickTime = now;

	const fs::RPC* rpcChunk = fs::getRPCMap().GetRPC("FileTransferChunk");
	if (!rpcChunk) return;

	for (int i = 0; i < CHUNKS_PER_TICK && !m_pendingChunks.empty(); ++i) {
		PendingChunk& pc = m_pendingChunks.front();
		if (pc.isServer)
			fs::sender.SendRPC(pc.targets, rpcChunk,
			                   pc.transferID, pc.chunkIndex, pc.data);
		else
			fs::sender.SendRPCRequest(pc.targets, rpcChunk,
			                          pc.transferID, pc.chunkIndex, pc.data);
		m_pendingChunks.pop_front();
	}
}

// ---------------------------------------------------------------------------

std::vector<FileTransferManager::ReceivedFile> FileTransferManager::PollCompleted() {
	std::lock_guard<std::mutex> lk(m_mutex);
	std::vector<ReceivedFile> out(m_completed.begin(), m_completed.end());
	m_completed.clear();
	return out;
}

std::vector<FileTransferManager::TransferProgress> FileTransferManager::PollProgress() {
	std::lock_guard<std::mutex> lk(m_mutex);
	std::vector<TransferProgress> out(m_progress.begin(), m_progress.end());
	m_progress.clear();
	return out;
}

void FileTransferManager::OnTransferStart(uint32_t transferID, uint8_t senderID,
                                          const std::string& filename, uint64_t totalSize,
                                          uint32_t totalChunks) {
	std::lock_guard<std::mutex> lk(m_mutex);
	auto& t       = m_incoming[transferID];
	t.senderID    = senderID;
	t.filename    = filename;
	t.totalSize   = totalSize;
	t.totalChunks = totalChunks;
}

void FileTransferManager::OnChunkReceived(uint32_t transferID, uint32_t chunkIndex,
                                          const std::vector<uint8_t>& data) {
	std::lock_guard<std::mutex> lk(m_mutex);
	auto it = m_incoming.find(transferID);
	if (it == m_incoming.end())
		return;

	it->second.chunks[chunkIndex] = data;

	m_progress.push_back({
		transferID,
		it->second.senderID,
		it->second.filename,
		static_cast<uint32_t>(it->second.chunks.size()),
		it->second.totalChunks
	});

	TryAssembleLocked(transferID);
}

bool FileTransferManager::TryAssembleLocked(uint32_t transferID) {
	auto it = m_incoming.find(transferID);
	if (it == m_incoming.end()) return false;

	auto& t = it->second;
	if (t.chunks.size() < t.totalChunks) return false;

	for (uint32_t i = 0; i < t.totalChunks; ++i) {
		if (t.chunks.find(i) == t.chunks.end()) return false;
	}

	std::vector<uint8_t> assembled;
	assembled.reserve(static_cast<size_t>(t.totalSize));
	for (uint32_t i = 0; i < t.totalChunks; ++i) {
		const auto& chunk = t.chunks[i];
		assembled.insert(assembled.end(), chunk.begin(), chunk.end());
	}

	std::error_code ec;
	std::filesystem::create_directories("received_files", ec);

	std::string savedPath = "received_files/" + t.filename;
	if (std::filesystem::exists(savedPath)) {
		const auto stem = std::filesystem::path(t.filename).stem().string();
		const auto ext  = std::filesystem::path(t.filename).extension().string();
		for (int n = 1; ; ++n) {
			std::string candidate =
				"received_files/" + stem + "_" + std::to_string(n) + ext;
			if (!std::filesystem::exists(candidate)) {
				savedPath = std::move(candidate);
				break;
			}
		}
	}

	{
		std::ofstream out(savedPath, std::ios::binary);
		if (out.is_open())
			out.write(reinterpret_cast<const char*>(assembled.data()),
			          static_cast<std::streamsize>(assembled.size()));
	}

	ReceivedFile rf;
	rf.senderID  = t.senderID;
	rf.filename  = t.filename;
	rf.savedPath = savedPath;
	rf.totalSize = t.totalSize;
	m_completed.push_back(std::move(rf));

	m_incoming.erase(it);
	return true;
}
