#pragma once

#include "spdlog/spdlog.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include <steam/steamclientpublic.h>

template <>
struct fmt::formatter<glm::vec2> {
	constexpr auto parse(format_parse_context& ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const glm::vec2& v, FormatContext& ctx) const {
		return fmt::format_to(ctx.out(), "({}, {})", v.x, v.y);
	}
};

template <>
struct fmt::formatter<glm::ivec2> {
	constexpr auto parse(format_parse_context& ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const glm::ivec2& v, FormatContext& ctx) const {
		return fmt::format_to(ctx.out(), "({}, {})", v.x, v.y);
	}
};

template <>
struct fmt::formatter<glm::vec3> {
	constexpr auto parse(format_parse_context& ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const glm::vec3& v, FormatContext& ctx) const {
		return fmt::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
	}
};

template <>
struct fmt::formatter<glm::vec4> {
	constexpr auto parse(format_parse_context& ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const glm::vec4& v, FormatContext& ctx) const {
		return fmt::format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
	}
};

template <>
struct fmt::formatter<CSteamID> {
	constexpr auto parse(format_parse_context& ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const CSteamID& v, FormatContext& ctx) const {
		return fmt::format_to(ctx.out(), "{}", v.ConvertToUint64());
	}
};

template <>
struct fmt::formatter<EResult> {
	constexpr auto parse(format_parse_context& ctx) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const EResult& v, FormatContext& ctx) const {
		return fmt::format_to(ctx.out(), "{}", static_cast<uint16_t>(v));
	}
};