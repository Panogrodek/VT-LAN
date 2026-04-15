#pragma once


#if USE_STEAM_GNS
#include <steam/steamclientpublic.h>
#include <steam/steamnetworkingtypes.h>
#include <steam/steam_api.h>
#else
#include <steam/steamnetworkingtypes.h>
#endif

namespace fs {
	namespace fs_priv {
		class Steam {
		public:

			bool InitializeSteam(); //TODO: error codes /or not
			void Shutdown();

			bool IsRunning() const;

			// Opens steam build in invite overlay, that you can invite friends over. Use after lobby is created
			bool OpenInviteOverlay();

			SteamNetworkingMicroseconds GetLocalTimestamp() const;
		private:
			bool m_initialized = false;
		};
	}

	inline fs_priv::Steam steam;
}