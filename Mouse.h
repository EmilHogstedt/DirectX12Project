#pragma once
class Mouse
{
public:
	static void OnRawDelta(const int dx, const int dy) noexcept;
	static const std::pair<int, int> GetMovementDelta() noexcept;
	static void OnRightMouseButtonPressed() noexcept;
	static void OnRightMouseButtonReleased() noexcept;
	static constexpr bool IsRightButtonPressed() noexcept { return m_RightButtonPressed; }
private:
	static int m_sDeltaX;
	static int m_sDeltaY;
	static bool m_RightButtonPressed;
};