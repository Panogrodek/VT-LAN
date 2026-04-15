#pragma once

/*
CALLBACK
TODO: check if return types other than void work correctly on callbacks

CHANNEL
TODO: check if sendign huge data works

CONNECTION
NOTE: to be fair, this is the most checked part of the code (due to there being the most amount of errors)
If this still fails, it will in production. Its the most fundamental and most simple class in whole networking, 
so debugging it will be easier then other components

NETWORKING
TODO: everything

RECEIVER
TODO: everything

RPC
TODO: everything

SENDER
TODO: everything
*/

namespace fs {
	namespace fs_priv {
		class NetworkingTest {
		public:
			void CallbackTest();
			void ChannelTestInit();
			void ConnectionTestInit();

			void Update();
			void BeforeReceive();
			void AfterReceive();
			void BeforeSend();
			void AfterSend();
		private:
			std::vector<std::function<void()>> m_update;
			std::vector<std::function<void()>> m_beforeReceive;
			std::vector<std::function<void()>> m_afterReceive;
			std::vector<std::function<void()>> m_beforeSend;
			std::vector<std::function<void()>> m_afterSend;
		};
	}

	inline fs_priv::NetworkingTest networkingTest;
}