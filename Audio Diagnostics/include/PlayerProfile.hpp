#pragma once
#include <string>
#include <fstream>

namespace PlayerProfile {
	inline std::string firstName;
	inline std::string lastName;

	inline void Load() {
		std::ifstream file("player_profile.txt");
		if (!file.is_open()) return;
		std::getline(file, firstName);
		std::getline(file, lastName);
	}

	inline void Save() {
		std::ofstream file("player_profile.txt");
		if (!file.is_open()) return;
		file << firstName << "\n" << lastName << "\n";
	}

	inline bool HasName() {
		return !firstName.empty() || !lastName.empty();
	}

	inline std::string FullName() {
		return firstName + (lastName.empty() ? "" : " " + lastName);
	}
}
