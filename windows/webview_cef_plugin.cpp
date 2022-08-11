#include "webview_cef_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>
#include <windowsx.h>
#include <shellscalingapi.h>

// for getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/standard_method_codec.h>

#include <iostream>

#include "include/cef_app.h"
#include "simple_handler.h"
#include "browser.h"
#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_result_functions.h>
#include <flutter/standard_method_codec.h>
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"
#include "data.h"

#include "main_message_loop_multithreaded_win.h"

#define DPI_1X 96.0f

namespace webview_cef
{
	HWND WebviewCefPlugin::hwnd = nullptr;
	SimpleHandler *WebviewCefPlugin::handler = nullptr;

	int DeviceToLogical(int value, float device_scale_factor)
	{
		float scaled_val = static_cast<float>(value) / device_scale_factor;
		return static_cast<int>(std::floor(scaled_val));
	}

	CefRect DeviceToLogical(const CefRect &value, float device_scale_factor)
	{
		return CefRect(DeviceToLogical(value.x, device_scale_factor),
					   DeviceToLogical(value.y, device_scale_factor),
					   DeviceToLogical(value.width, device_scale_factor),
					   DeviceToLogical(value.height, device_scale_factor));
	}

	void DeviceToLogical(CefMouseEvent &value, float device_scale_factor)
	{
		value.x = DeviceToLogical(value.x, device_scale_factor);
		value.y = DeviceToLogical(value.y, device_scale_factor);
	}

	bool IsProcessPerMonitorDpiAware()
	{
		enum class PerMonitorDpiAware
		{
			UNKNOWN = 0,
			PER_MONITOR_DPI_UNAWARE,
			PER_MONITOR_DPI_AWARE,
		};
		static PerMonitorDpiAware per_monitor_dpi_aware = PerMonitorDpiAware::UNKNOWN;
		if (per_monitor_dpi_aware == PerMonitorDpiAware::UNKNOWN)
		{
			per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_UNAWARE;
			HMODULE shcore_dll = ::LoadLibrary(L"shcore.dll");
			if (shcore_dll)
			{
				typedef HRESULT(WINAPI * GetProcessDpiAwarenessPtr)(
					HANDLE, PROCESS_DPI_AWARENESS *);
				GetProcessDpiAwarenessPtr func_ptr =
					reinterpret_cast<GetProcessDpiAwarenessPtr>(
						::GetProcAddress(shcore_dll, "GetProcessDpiAwareness"));
				if (func_ptr)
				{
					PROCESS_DPI_AWARENESS awareness;
					if (SUCCEEDED(func_ptr(nullptr, &awareness)) &&
						awareness == PROCESS_PER_MONITOR_DPI_AWARE)
						per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
				}
			}
		}
		return per_monitor_dpi_aware == PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
	}

	float GetDeviceScaleFactor()
	{
		static float scale_factor = 1.0;
		static bool initialized = false;

		if (!initialized)
		{
			// This value is safe to cache for the life time of the app since the user
			// must logout to change the DPI setting. This value also applies to all
			// screens.
			HDC screen_dc = ::GetDC(nullptr);
			int dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
			scale_factor = static_cast<float>(dpi_x) / 96.0f;
			::ReleaseDC(nullptr, screen_dc);
			initialized = true;
		}

		return scale_factor;
	}

	float GetWindowScaleFactor()
	{
		if (IsProcessPerMonitorDpiAware())
		{
			typedef UINT(WINAPI * GetDpiForWindowPtr)(HWND);
			static GetDpiForWindowPtr func_ptr = reinterpret_cast<GetDpiForWindowPtr>(
				GetProcAddress(GetModuleHandle(L"user32.dll"), "GetDpiForWindow"));
			if (func_ptr)
				return static_cast<float>(func_ptr(WebviewCefPlugin::hwnd)) / DPI_1X;
		}

		return GetDeviceScaleFactor();
	}

	int GetCefMouseModifiers(WPARAM wparam)
	{
		int modifiers = 0;
		if (wparam & MK_CONTROL)
			modifiers |= EVENTFLAG_CONTROL_DOWN;
		if (wparam & MK_SHIFT)
			modifiers |= EVENTFLAG_SHIFT_DOWN;
		if (IsKeyDown(VK_MENU))
			modifiers |= EVENTFLAG_ALT_DOWN;
		if (wparam & MK_LBUTTON)
			modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
		if (wparam & MK_MBUTTON)
			modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
		if (wparam & MK_RBUTTON)
			modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

		// Low bit set from GetKeyState indicates "toggled".
		if (::GetKeyState(VK_NUMLOCK) & 1)
			modifiers |= EVENTFLAG_NUM_LOCK_ON;
		if (::GetKeyState(VK_CAPITAL) & 1)
			modifiers |= EVENTFLAG_CAPS_LOCK_ON;
		return modifiers;
	}

	WebviewCefPlugin *g_instance = nullptr;
	CefRefPtr<SimpleHandler> handler(new SimpleHandler());

	void runLoop()
	{
		// Create the main message loop object.
		std::unique_ptr<client::MainMessageLoop> message_loop;
		message_loop.reset(new client::MainMessageLoopMultithreadedWin);

		// Run the message loop. This will block but run in hidden window!
		message_loop->Run();
		message_loop.reset();
	}

	// static
	void WebviewCefPlugin::RegisterWithRegistrar(
		flutter::PluginRegistrarWindows *registrar)
	{
		auto channel =
			std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
				registrar->messenger(), "webview_cef",
				&flutter::StandardMethodCodec::GetInstance());

		auto plugin = std::make_unique<WebviewCefPlugin>(registrar->texture_registrar(), registrar->messenger());

		channel->SetMethodCallHandler(
			[plugin_pointer = plugin.get()](const auto &call, auto result)
			{
				plugin_pointer->HandleMethodCall(call, std::move(result));
			});

		registrar->AddPlugin(std::move(plugin));
		new std::thread(runLoop);
		LOG(INFO) << "Successfully loaded webview_cef plugin" << std::endl;
	}

	WebviewCefPlugin::WebviewCefPlugin(
		flutter::TextureRegistrar *textures,
		flutter::BinaryMessenger *messenger) : textures_(textures), messenger_(messenger),
											   mouse_tracking_(false),
											   last_click_x_(0),
											   last_click_y_(0),
											   last_click_button_(MBT_LEFT),
											   last_click_count_(1),
											   last_click_time_(0),
											   mouse_rotation_(false)
	{
		DCHECK(!g_instance);
		g_instance = this;
		event_channel_ =
			std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
				messenger, "webview_cef_events",
				&flutter::StandardMethodCodec::GetInstance());
		auto handler2 = std::make_unique<
			flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
			[this](const flutter::EncodableValue *arguments,
				   std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&
					   events)
			{
				event_sink_ = std::move(events);
				return nullptr;
			},
			[this](const flutter::EncodableValue *arguments)
			{
				event_sink_ = nullptr;
				return nullptr;
			});

		event_channel_->SetStreamHandler(std::move(handler2));
	}

	WebviewCefPlugin::~WebviewCefPlugin()
	{
		g_instance = nullptr;
	}

	void WebviewCefPlugin::HandleMethodCall(
		const flutter::MethodCall<flutter::EncodableValue> &method_call,
		std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
	{
		if (method_call.method_name().compare("createBrowser") == 0)
		{
			const flutter::EncodableMap *args = std::get_if<flutter::EncodableMap>(method_call.arguments());
			auto url_encoded = (args->find(flutter::EncodableValue("startUrl")))->second;
			auto bind_func_encoded = (args->find(flutter::EncodableValue("webMessageFunction")))->second;
			auto token_encoded = (args->find(flutter::EncodableValue("token")))->second;
			auto access_token_encoded = (args->find(flutter::EncodableValue("accessToken")))->second;
			auto is_string_encoded = (args->find(flutter::EncodableValue("isHTML")))->second;
			if (std::holds_alternative<std::string>(url_encoded) && std::holds_alternative<std::string>(bind_func_encoded),
				std::holds_alternative<bool>(is_string_encoded))
			{
				std::string url = std::get<std::string>(url_encoded);
				std::string bind_func = std::get<std::string>(bind_func_encoded);
				std::string token = std::get<std::string>(token_encoded);
				std::string access_token = std::get<std::string>(access_token_encoded);
				bool is_string = std::get<bool>(is_string_encoded);
				if (is_string)
				{
					url = GetDataURI(url, "text/html");
				}
				int64_t texture_id = handler->createBrowser(messenger_, textures_, url, bind_func, token, access_token);
				LOG(INFO) << "Create browser request for " << texture_id << " texture";
				return result->Success(flutter::EncodableValue(texture_id));
			}
		}

		else if (method_call.method_name().compare("closeAllBrowsers") == 0)
		{
			if (const auto force = std::get_if<bool>(method_call.arguments()))
			{
				handler->CloseAllBrowsers(*force);
				return result->Success();
			}
		}
		else if (method_call.method_name().compare("shutdown") == 0)
		{
			CefShutdown();
			return result->Success();
		}
		else
		{
			return result->NotImplemented();
		}
	}

	void
	WebviewCefPlugin::sendKeyboardEvent(UINT message, WPARAM wParamSource, LPARAM lParamSource)
	{
		if (!handler)
		{
			return;
		}
		int wParam = int(wParamSource);
		int lParam = int(lParamSource);

		CefKeyEvent event;
		event.windows_key_code = wParam;
		event.native_key_code = lParam;
		event.is_system_key = message == WM_SYSCHAR ||
							  message == WM_SYSKEYDOWN ||
							  message == WM_SYSKEYUP;

		if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
			event.type = KEYEVENT_RAWKEYDOWN;
		else if (message == WM_KEYUP || message == WM_SYSKEYUP)
			event.type = KEYEVENT_KEYUP;
		else
			event.type = KEYEVENT_CHAR;

		int modifiers = 0;
		if (IsKeyDown(VK_SHIFT))
			modifiers |= EVENTFLAG_SHIFT_DOWN;
		if (IsKeyDown(VK_CONTROL))
			modifiers |= EVENTFLAG_CONTROL_DOWN;
		if (IsKeyDown(VK_MENU))
			modifiers |= EVENTFLAG_ALT_DOWN;

		// Low bit set from GetKeyState indicates "toggled".
		if (::GetKeyState(VK_NUMLOCK) & 1)
			modifiers |= EVENTFLAG_NUM_LOCK_ON;
		if (::GetKeyState(VK_CAPITAL) & 1)
			modifiers |= EVENTFLAG_CAPS_LOCK_ON;

		switch (wParam)
		{
		case VK_RETURN:
			if ((lParam >> 16) & KF_EXTENDED)
				modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_INSERT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
			if (!((lParam >> 16) & KF_EXTENDED))
				modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_NUMLOCK:
		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9:
		case VK_DIVIDE:
		case VK_MULTIPLY:
		case VK_SUBTRACT:
		case VK_ADD:
		case VK_DECIMAL:
		case VK_CLEAR:
			modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_SHIFT:
			if (IsKeyDown(VK_LSHIFT))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (IsKeyDown(VK_RSHIFT))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_CONTROL:
			if (IsKeyDown(VK_LCONTROL))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (IsKeyDown(VK_RCONTROL))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_MENU:
			if (IsKeyDown(VK_LMENU))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (IsKeyDown(VK_RMENU))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_LWIN:
			modifiers |= EVENTFLAG_IS_LEFT;
			break;
		case VK_RWIN:
			modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		}
		event.modifiers = modifiers;
		event.focus_on_editable_field = true;
		handler->sendKeyEvent(event);
	}

	void
	WebviewCefPlugin::sendMouseEvent(UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (!handler || !hwnd)
		{
			return;
		}

		LONG currentTime = 0;
		bool cancelPreviousClick = false;

		if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
			message == WM_MBUTTONDOWN || message == WM_MOUSEMOVE)
		{
			currentTime = GetMessageTime();
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			cancelPreviousClick =
				(abs(last_click_x_ - x) > (GetSystemMetrics(SM_CXDOUBLECLK) / 2)) ||
				(abs(last_click_y_ - y) > (GetSystemMetrics(SM_CYDOUBLECLK) / 2)) ||
				((currentTime - last_click_time_) > GetDoubleClickTime());
			if (cancelPreviousClick &&
				(message == WM_MOUSEMOVE))
			{
				last_click_count_ = 1;
				last_click_x_ = 0;
				last_click_y_ = 0;
				last_click_time_ = 0;
			}
		}

		switch (message)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			if (wParam & MK_SHIFT)
			{
				// Start rotation effect.
				last_mouse_pos_.x = current_mouse_pos_.x = x;
				last_mouse_pos_.y = current_mouse_pos_.y = y;
				mouse_rotation_ = true;
			}
			else
			{
				CefBrowserHost::MouseButtonType btnType =
					(message == WM_LBUTTONDOWN
						 ? MBT_LEFT
						 : (message == WM_RBUTTONDOWN ? MBT_RIGHT : MBT_MIDDLE));
				if (!cancelPreviousClick && (btnType == last_click_button_))
				{
					++last_click_count_;
				}
				else
				{
					last_click_count_ = 1;
					last_click_x_ = x;
					last_click_y_ = y;
				}
				last_click_time_ = currentTime;
				last_click_button_ = btnType;

				if (last_click_count_ <= 2)
				{
					CefMouseEvent mouse_event;
					mouse_event.x = x;
					mouse_event.y = y;
					DeviceToLogical(mouse_event, GetDeviceScaleFactor());
					mouse_event.modifiers = GetCefMouseModifiers(wParam);
					handler->sendMouseClickEvent(mouse_event, btnType, false,
												 last_click_count_);
				}
			}
		}
		break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			if (mouse_rotation_)
			{
				mouse_rotation_ = false;
			}
			else
			{
				int x = GET_X_LPARAM(lParam);
				int y = GET_Y_LPARAM(lParam);
				CefBrowserHost::MouseButtonType btnType =
					(message == WM_LBUTTONUP
						 ? MBT_LEFT
						 : (message == WM_RBUTTONUP ? MBT_RIGHT : MBT_MIDDLE));

				if (last_click_count_ <= 2)
				{
					CefMouseEvent mouse_event;
					mouse_event.x = x;
					mouse_event.y = y;
					DeviceToLogical(mouse_event, GetDeviceScaleFactor());
					mouse_event.modifiers = GetCefMouseModifiers(wParam);
					handler->sendMouseClickEvent(mouse_event, btnType, true,
												 last_click_count_);
				}
			}

			break;
		}
		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			if (mouse_rotation_)
			{
				// Apply rotation effect.
				current_mouse_pos_.x = x;
				current_mouse_pos_.y = y;
				last_mouse_pos_.x = current_mouse_pos_.x;
				last_mouse_pos_.y = current_mouse_pos_.y;
			}
			else
			{

				CefMouseEvent mouse_event;
				mouse_event.x = x;
				mouse_event.y = y;
				DeviceToLogical(mouse_event, GetDeviceScaleFactor());
				mouse_event.modifiers = GetCefMouseModifiers(wParam);
				handler->sendMouseMoveEvent(mouse_event, false);
			}
			break;
		}
		}
	}

	// static
	WebviewCefPlugin *WebviewCefPlugin::GetInstance()
	{
		return g_instance;
	}

} // namespace webview_cef
