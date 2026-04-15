#pragma once
#include <steam/steamclientpublic.h>
#include <unordered_map>
#include <cstdint>

// CSteamID
namespace std {
	template<>
	struct hash<CSteamID> {
		std::size_t operator()(CSteamID const& steamID) const noexcept {
			return steamID.ConvertToUint64();
		}
	};
}