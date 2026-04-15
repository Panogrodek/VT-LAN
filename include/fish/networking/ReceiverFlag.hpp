#pragma once

namespace fs {
    class RPC;

    enum class ReceiverFlag : uint16_t {
        Invalid	 = 0,
        Host	 = 1 << 0,
        Player2	 = 1 << 1,
        Player3	 = 1 << 2,
        Player4	 = 1 << 3,
        Player5	 = 1 << 4,
        Player6	 = 1 << 5,
        Player7	 = 1 << 6,
        Player8	 = 1 << 7,
        Player9	 = 1 << 8,
        Player10 = 1 << 9,
        Player11 = 1 << 10,
        Player12 = 1 << 11,
        Player13 = 1 << 12,
        Player14 = 1 << 13,
        Player15 = 1 << 14,
        Player16 = 1 << 15,

		All = Host | Player2 | Player3 | Player4 | Player5 | Player6 |
		Player7 | Player8 | Player9 | Player10 | Player11 |
		Player12 | Player13 | Player14 | Player15 | Player16
    };
	// ------------------------------
	// Bitwise operators
	// ------------------------------
	inline ReceiverFlag operator|(ReceiverFlag lhs, ReceiverFlag rhs) {
		return static_cast<ReceiverFlag>(
			static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs)
			);
	}

	inline ReceiverFlag operator&(ReceiverFlag lhs, ReceiverFlag rhs) {
		return static_cast<ReceiverFlag>(
			static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs)
			);
	}

	inline ReceiverFlag operator^(ReceiverFlag lhs, ReceiverFlag rhs) {
		return static_cast<ReceiverFlag>(
			static_cast<uint16_t>(lhs) ^ static_cast<uint16_t>(rhs)
			);
	}

	inline ReceiverFlag operator~(ReceiverFlag flag) {
		return static_cast<ReceiverFlag>(
			~static_cast<uint16_t>(flag)
			);
	}

	inline ReceiverFlag& operator|=(ReceiverFlag& lhs, ReceiverFlag rhs) {
		lhs = lhs | rhs;
		return lhs;
	}

	inline ReceiverFlag& operator&=(ReceiverFlag& lhs, ReceiverFlag rhs) {
		lhs = lhs & rhs;
		return lhs;
	}

	inline ReceiverFlag& operator^=(ReceiverFlag& lhs, ReceiverFlag rhs) {
		lhs = lhs ^ rhs;
		return lhs;
	}

	inline bool HasFlag(ReceiverFlag flags, ReceiverFlag test) {
		return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(test)) != 0;
	}

	inline bool HasAllFlags(ReceiverFlag flags, ReceiverFlag test) {
		return (static_cast<uint16_t>(flags) & static_cast<uint16_t>(test))
			== static_cast<uint16_t>(test);
	}

	inline void ClearFlag(ReceiverFlag& flags, ReceiverFlag toClear) {
		flags = static_cast<ReceiverFlag>(
			static_cast<uint16_t>(flags) & ~static_cast<uint16_t>(toClear)
			);
	}

	inline std::vector<ReceiverFlag> GetAllReceiversInFlag(ReceiverFlag flags) {
		std::vector<ReceiverFlag> out;
		for (uint16_t i = 0; i < 16u; i++) {
			ReceiverFlag bit = static_cast<ReceiverFlag>(1u << i);
			if (HasFlag(flags, bit))
				out.push_back(bit);
		}
		return out;
	}

	inline ReceiverFlag GameIDToReceiverFlag(uint8_t id) {
		if (id == 0 || id > 16)
			return ReceiverFlag::Invalid;
		return static_cast<ReceiverFlag>(1u << (id - 1));
	}

    inline uint8_t ReceiverFlagToGameID(ReceiverFlag flag) {
        auto value = static_cast<uint16_t>(flag);
        if (value == 0 || (value & (value - 1)) != 0)
            return 0;
        return static_cast<uint8_t>(std::log2(value)) + 1;
    }


    inline constexpr ReceiverFlag HostFlag = ReceiverFlag::Host;
	inline constexpr ReceiverFlag AllFlag = ReceiverFlag::All;

	namespace fs_priv {
		class ReceiverFlagManager {
		public:
			static void SetMeFlag(fs::ReceiverFlag flag) {
				s_meFlag.store(static_cast<uint16_t>(flag), std::memory_order_release);
			}

			static void SetAllCurrentReceivers(ReceiverFlag flags) {
				s_allCurrentReceivers.store(static_cast<uint16_t>(flags), std::memory_order_release);
			}

			static ReceiverFlag GetMeFlag() {
				return static_cast<ReceiverFlag>(s_meFlag.load(std::memory_order_acquire));
			}

			static ReceiverFlag GetAllCurrentReceivers() {
				return static_cast<ReceiverFlag>(s_allCurrentReceivers.load(std::memory_order_acquire));
			}
		private:
			static std::atomic<uint16_t> s_allCurrentReceivers;
			static std::atomic<uint16_t> s_meFlag;
		};

		inline ReceiverFlagManager receiverFlagManager;
	}

	inline ReceiverFlag MeFlag() {
		return fs_priv::receiverFlagManager.GetMeFlag();
	}

	inline ReceiverFlag AllCurrentReceivers() {
		return fs_priv::receiverFlagManager.GetAllCurrentReceivers();
	}
}
