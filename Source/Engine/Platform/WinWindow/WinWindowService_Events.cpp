#include "WinWindowService.h"

#include "../../Common/LogService.h"
#include "../../Services/HIDService.h"

#include "../../Engine.h"

using namespace Inno;

void WinWindowService::ConsumeEvents(const WindowEventProcessCallback& p_Callback)
{
	m_WindowEvents.Read([&](auto const& l_FrontBuffer)
		{
			p_Callback(l_FrontBuffer);
		});

	m_WindowEvents.Flip();
	m_WindowEvents.Write([](auto& l_BackBuffer)
		{
			for (auto i : l_BackBuffer)
			{
				delete i;
			}
			l_BackBuffer.clear();
		});
}

static uint32_t TranslateVKCode(uint32_t vk)
{
	switch (vk)
	{
	case VK_LSHIFT:   return INNO_KEY_LEFT_SHIFT;
	case VK_RSHIFT:   return INNO_KEY_RIGHT_SHIFT;
	case VK_LCONTROL: return INNO_KEY_LEFT_CONTROL;
	case VK_RCONTROL: return INNO_KEY_RIGHT_CONTROL;
	case VK_LMENU:    return INNO_KEY_LEFT_ALT;
	case VK_RMENU:    return INNO_KEY_RIGHT_ALT;
	default:          return vk;
	}
}

bool WinWindowService::SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
	for (auto i : m_WindowEventCallbacks)
	{
		(*i)(windowHook, uMsg, (uint64_t)wParam, (int64_t)lParam);
	}

	HWND hwnd = (HWND)windowHook;
	switch (uMsg)
	{
	case WM_DESTROY:
	{
		Log(Warning, "WM_DESTROY signal received.");
		m_ObjectStatus = ObjectStatus::Suspended;
		return true;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
		return true;
	}
	case WM_SIZE:
	{
		if (lParam && g_Engine->GetStatus() == ObjectStatus::Activated)
		{
			auto l_width = lParam & 0xffff;
			auto l_height = (lParam & 0xffff0000) >> 16;

			TVec2<uint32_t> l_newResolution = TVec2<uint32_t>((uint32_t)l_width, (uint32_t)l_height);
			Log(Success, "WinWindowService: WM_SIZE ", l_newResolution.x, "x", l_newResolution.y);
			g_Engine->Get<HIDService>()->WindowResizeCallback(l_newResolution.x, l_newResolution.y);

			return true;
		}

		return false;
	}
	case WM_KEYDOWN:
	{
		auto l_buttonState = new ButtonState(TranslateVKCode((uint32_t)wParam), true);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});

		return true;
	}
	case WM_KEYUP:
	{
		auto l_buttonState = new ButtonState(TranslateVKCode((uint32_t)wParam), false);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_LBUTTONDOWN:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_LEFT, true);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_LBUTTONUP:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_LEFT, false);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_RBUTTONDOWN:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_RIGHT, true);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_RBUTTONUP:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_RIGHT, false);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_MOUSEMOVE:
	{
		auto l_mouseState = new MouseState(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_mouseState);
			});
		return true;
	}
	}
	return false;
}

bool WinWindowService::AddEventCallback(WindowEventCallback* callback)
{
	m_WindowEventCallbacks.emplace(callback);
	return true;
}

LRESULT CALLBACK WinWindowService::WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto l_processed = g_Engine->getWindowService()->SendEvent(hwnd, uMsg, wParam, lParam);
	if (l_processed)
	{
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
