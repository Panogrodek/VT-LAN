#pragma once

namespace fs {
	enum class AngleType {
		Degrees,
		Radians,
	};

	class Angle {
	public:
		Angle();
		Angle(AngleType type, double value);
		~Angle() = default;

		Angle& operator+=(const Angle& other);
		Angle& operator-=(const Angle& other);

		bool operator==(const Angle& other);
		bool operator!=(const Angle& other);

		bool operator> (const Angle& other);
		bool operator>=(const Angle& other);

		bool operator< (const Angle& other);
		bool operator<=(const Angle& other);

		double AsDegrees() const;
		double AsRadians() const;

	private:
		AngleType m_type = AngleType::Degrees;

		double m_value = 0.0;

		double DegreesToRadians() const;
		double RadiansToDegrees() const;
	};

	Angle Degrees(double value);
	Angle Radians(double value);
}