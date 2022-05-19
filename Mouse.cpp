#include "pch.h"
#include "Mouse.h"

int Mouse::m_sDeltaX{ 0 };
int Mouse::m_sDeltaY{ 0 };
bool Mouse::m_RightButtonPressed{ false };

void Mouse::OnRawDelta(const int dx, const int dy) noexcept
{
	m_sDeltaX = dx;
	m_sDeltaY = dy;
}

const std::pair<int, int> Mouse::GetMovementDelta() noexcept
{
	int returnX = m_sDeltaX;
	int returnY = m_sDeltaY;

	m_sDeltaX = 0;
	m_sDeltaY = 0;
	return std::pair<int, int>(returnX, returnY);
}

void Mouse::OnRightMouseButtonPressed() noexcept
{
	m_RightButtonPressed = true;
}

void Mouse::OnRightMouseButtonReleased() noexcept
{
	m_RightButtonPressed = false;
}
