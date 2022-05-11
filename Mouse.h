#pragma once
class Mouse
{
public:
	static void OnRawDelta(const int dx, const int dy) noexcept;
	static const std::pair<int, int> GetMovementDelta() noexcept;
private:
	static int m_sDeltaX;
	static int m_sDeltaY;
};