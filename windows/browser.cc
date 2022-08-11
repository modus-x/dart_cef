// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "browser.h"

#include <string>
#include <fmt/core.h>
#include <optional>

#include <flutter/event_stream_handler_functions.h>
#include <flutter/method_result_functions.h>
#include <flutter/standard_method_codec.h>

#include "data.h"
#include "renderer_delegate.h"
#include "simple_handler.h"
#include "include/wrapper/cef_helpers.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

namespace
{

  class WebviewCookieClearCompletionCallBack : public CefDeleteCookiesCallback
  {
  public:
    WebviewCookieClearCompletionCallBack(WebviewEvent event, CefRefPtr<BrowserBridge> bridge) : event_(event), bridge_(bridge){};

    void OnComplete(int num) override
    {
      bridge_->browserEvent(event_);
    }

  private:
    WebviewEvent event_;
    CefRefPtr<BrowserBridge> bridge_;

    IMPLEMENT_REFCOUNTING(WebviewCookieClearCompletionCallBack);
  };

  static const std::string &GetCursorName(const cef_cursor_type_t cursor)
  {

    static const std::string kDefaultCursorName = "basic";
    static const std::map<cef_cursor_type_t, std::string> cursors = {
        {CT_NORTHWESTSOUTHEASTRESIZE, "allScroll"},
        {CT_POINTER, kDefaultCursorName},
        {CT_HAND, "click"},
        {CT_NOTALLOWED, "forbidden"},
        {CT_HELP, "help"},
        {CT_MOVE, "move"},
        {CT_NONE, "none"},
        {CT_NODROP, "noDrop"},
        {CT_CROSS, "precise"},
        {CT_PROGRESS, "progress"},
        {CT_IBEAM, "text"},
        {CT_COLUMNRESIZE, "resizeColumn"},
        {CT_SOUTHRESIZE, "resizeDown"},
        {CT_SOUTHWESTRESIZE, "resizeDownLeft"},
        {CT_SOUTHEASTRESIZE, "resizeDownRight"},
        {CT_WESTRESIZE, "resizeLeft"},
        {CT_EASTWESTRESIZE, "resizeLeftRight"},
        {CT_EASTRESIZE, "resizeRight"},
        {CT_ROWRESIZE, "resizeRow"},
        {CT_NORTHRESIZE, "resizeUp"},
        {CT_NORTHSOUTHRESIZE, "resizeUpDown"},
        {CT_NORTHWESTRESIZE, "resizeUpLeft"},
        {CT_NORTHEASTRESIZE, "resizeUpRight"},
        {CT_NORTHWESTSOUTHEASTRESIZE, "resizeUpLeftDownRight"},
        {CT_NORTHEASTSOUTHWESTRESIZE, "resizeUpRightDownLeft"},
        {CT_WAIT, "wait"},
        {CT_NONE, "none"},
        {CT_ZOOMIN, "zoomIn"},
        {CT_ZOOMOUT, "zoomOut"},
        {CT_COPY, "copy"},
        {CT_ALIAS, "alias"},
        {CT_GRABBING, "grabbing"},
        {CT_GRAB, "grab"},
        {CT_CELL, "cell"},
        {CT_VERTICALTEXT, "verticalText"},
        {CT_CONTEXTMENU, "contextMenu"}};

    const auto it = cursors.find(cursor);
    if (it != cursors.end())
    {
      return it->second;
    }
    return kDefaultCursorName;
  }

  static const std::optional<std::pair<int, int>> GetPointFromArgs(
      const flutter::EncodableValue *args)
  {
    const flutter::EncodableList *list =
        std::get_if<flutter::EncodableList>(args);
    if (!list || list->size() != 2)
    {
      return std::nullopt;
    }
    const auto x = std::get_if<int>(&(*list)[0]);
    const auto y = std::get_if<int>(&(*list)[1]);
    if (!x || !y)
    {
      return std::nullopt;
    }
    return std::make_pair(*x, *y);
  }

  uint32 parseModifiers(std::string &&checkModifiers)
  {
    uint32 modifiers = 0;
    if (checkModifiers.find("shift;") != std::string::npos)
      modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (checkModifiers.find("capslock;") != std::string::npos)
      modifiers |= EVENTFLAG_CAPS_LOCK_ON;
    if (checkModifiers.find("control;") != std::string::npos)
      modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (checkModifiers.find("alt;") != std::string::npos)
      modifiers |= EVENTFLAG_ALT_DOWN;
    return modifiers;
  }

  template <typename T>
  std::optional<T> GetOptionalValue(const flutter::EncodableMap &map,
                                    const std::string &key)
  {
    const auto it = map.find(flutter::EncodableValue(key));
    if (it != map.end())
    {
      const auto val = std::get_if<T>(&it->second);
      if (val)
      {
        return *val;
      }
    }
    return std::nullopt;
  }
}

void newBrowserInstance(int64_t texture_id, const CefString &initialUrl, const CefString &bind_func, const CefString &token, const CefString &access_token)
{
  if (!CefCurrentlyOn(TID_UI))
  {
    CefPostTask(TID_UI, base::BindOnce(newBrowserInstance, texture_id, initialUrl, bind_func, token, access_token));
    return;
  }
  else
  {
    CefWindowInfo window_info;

#if defined(OS_WIN)
    // On Windows we need to specify certain flags that will be passed to
    // CreateWindowEx().
    window_info.SetAsWindowless(nullptr);
#endif
    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = 60;
    CefRefPtr<CefDictionaryValue> extra = CefDictionaryValue::Create();
    extra->SetString("texture_id", std::to_string(texture_id));
    extra->SetString("bind_func", bind_func);
    extra->SetString("token", token);
    extra->SetString("access_token", access_token);
    CefBrowserHost::CreateBrowser(window_info, SimpleHandler::GetInstance(),
                                  initialUrl, browser_settings,
                                  extra, nullptr);
    LOG(INFO) << "Sent request to create browser for texture " << texture_id << " with initial url " << initialUrl;
  }
}

BrowserBridge::BrowserBridge(
    flutter::BinaryMessenger *messenger,
    flutter::TextureRegistrar *texture_registrar, const CefString &url, const CefString &bind_func, const CefString &token, const CefString &access_token)
    : texture_registrar_(texture_registrar)
{
  texture_bridge = std::make_unique<Texture>();
  texture_bridge_pet = std::make_unique<Texture>();

  flutter_texture_ =
      std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
          [bridge = static_cast<Texture *>(texture_bridge.get())](
              size_t width, size_t height) -> const FlutterDesktopPixelBuffer *
          {
            return bridge->CopyPixelBuffer(width, height);
          }));
  flutter_texture_pet_ =
      std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
          [bridge = static_cast<Texture *>(texture_bridge_pet.get())](
              size_t width, size_t height) -> const FlutterDesktopPixelBuffer *
          {
            return bridge->CopyPixelBuffer(width, height);
          }));

  texture_id_ = texture_registrar->RegisterTexture(flutter_texture_.get());
  texture_id_pet_ = texture_registrar->RegisterTexture(flutter_texture_pet_.get());

  texture_bridge->setOnFrameAvailable(
      [this]()
      { texture_registrar_->MarkTextureFrameAvailable(texture_id_); });

  texture_bridge_pet->setOnFrameAvailable(
      [this]()
      { texture_registrar_->MarkTextureFrameAvailable(texture_id_pet_); });

  const auto method_channel_name =
      fmt::format("webview_cef/{}", texture_id_);
  method_channel_ =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          messenger, method_channel_name,
          &flutter::StandardMethodCodec::GetInstance());
  method_channel_->SetMethodCallHandler([this](const auto &call, auto result)
                                        { HandleMethodCall(call, std::move(result)); });

  const auto event_channel_name =
      fmt::format("webview_cef/{}/events", texture_id_);
  event_channel_ =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, event_channel_name,
          &flutter::StandardMethodCodec::GetInstance());
  auto handler = std::make_unique<
      flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
      [this, texture = texture_id_, initialUrl = url, bind_func = bind_func, token = token, access_token = access_token](const flutter::EncodableValue *arguments,
                                                                                                                         std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&
                                                                                                                             events)
      {
        event_sink_ = std::move(events);
        newBrowserInstance(texture, initialUrl, bind_func, token, access_token);
        return nullptr;
      },
      [this](const flutter::EncodableValue *arguments)
      {
        event_sink_ = nullptr;
        return nullptr;
      });

  event_channel_->SetStreamHandler(std::move(handler));
}

void BrowserBridge::setBrowser(CefRefPtr<CefBrowser> &browser)
{
  browser_ = browser;
}

void BrowserBridge::clearAllCookies()
{
  CefRefPtr<CefDeleteCookiesCallback> callback =
      new WebviewCookieClearCompletionCallBack(WebviewEvent::CookiesCleared, this);
  CefCookieManager::GetGlobalManager(nullptr)->DeleteCookies("", "", callback);
}

BrowserBridge::~BrowserBridge()
{
  method_channel_->SetMethodCallHandler(nullptr);
  texture_registrar_->UnregisterTexture(texture_id_);
}

void BrowserBridge::OnLoadingStateChange(
    bool isLoading,
    bool canGoBack,
    bool canGoForward)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("loadingState")},
      {flutter::EncodableValue(kEventValue),
       flutter::EncodableValue(isLoading ? static_cast<int>(WebviewLoadingState::InProcess)
                                         : static_cast<int>(WebviewLoadingState::NavigationCompleted))},
  });
  EmitEvent(event);
}

void BrowserBridge::mainFrameLoaded(int statusCode)
{
}

void BrowserBridge::onPopupShow(bool show)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("popupShow")},
      {flutter::EncodableValue(kEventValue),
       flutter::EncodableValue(show)},
  });
  EmitEvent(event);
}

void BrowserBridge::OnPopupSize(int x,
                                int y,
                                int popupWidth,
                                int popupHeight)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("popupSize")},
      {flutter::EncodableValue("x"),
       flutter::EncodableValue(x)},
      {flutter::EncodableValue("y"),
       flutter::EncodableValue(y)},
      {flutter::EncodableValue("width"),
       flutter::EncodableValue(popupWidth)},
      {flutter::EncodableValue("height"),
       flutter::EncodableValue(popupHeight)},
  });
  EmitEvent(event);
}

void BrowserBridge::OnWebMessage(const CefString &message)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("webMessage")},
      {flutter::EncodableValue(kEventValue),
       flutter::EncodableValue(message.ToString())},
  });
  EmitEvent(event);
}

void BrowserBridge::OnAfterCreated()
{
  OnWebviewStateChange(WebviewState::Ready);
}

void BrowserBridge::setToken(std::string token)
{
  CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(client::renderer::kTokenUpdate);
  CefRefPtr<CefListValue> args = message->GetArgumentList();
  args->SetString(0, token);
  browser_->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}

void BrowserBridge::setAccessToken(std::string token)
{
  CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(client::renderer::kAccessTokenUpdate);
  CefRefPtr<CefListValue> args = message->GetArgumentList();
  args->SetString(0, token);
  browser_->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}

void BrowserBridge::OnShutdown()
{
  OnWebviewStateChange(WebviewState::Shutdown);
}

void BrowserBridge::OnTitleChanged(const CefString &title)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("titleChanged")},
      {flutter::EncodableValue(kEventValue), flutter::EncodableValue(title.ToString())},
  });
  EmitEvent(event);
}

void BrowserBridge::textSelectionReport(const CefString &url)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("textSelectionReport")},
      {flutter::EncodableValue(kEventValue), flutter::EncodableValue(url.ToString())},
  });
  EmitEvent(event);
}

void BrowserBridge::onCursorChanged(const cef_cursor_type_t cursor)
{
  const auto &name = GetCursorName(cursor);
  const auto event = flutter::EncodableValue(
      flutter::EncodableMap{{flutter::EncodableValue(kEventType),
                             flutter::EncodableValue("cursorChanged")},
                            {flutter::EncodableValue(kEventValue), name}});
  EmitEvent(event);
}

void BrowserBridge::OnUrlChanged(const CefString &url)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("urlChanged")},
      {flutter::EncodableValue(kEventValue), flutter::EncodableValue(url.ToString())},
  });
  EmitEvent(event);
}

void BrowserBridge::browserEvent(WebviewEvent event)
{
  const auto dartEvent = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("browserEvent")},
      {flutter::EncodableValue(kEventValue),
       flutter::EncodableValue(static_cast<int>(event))},
  });
  EmitEvent(dartEvent);
}

// TODO: can be called from multiple parts of application (e.g. webview can be broken somewhere unexpectedly)
void BrowserBridge::OnWebviewStateChange(WebviewState state)
{
  const auto event = flutter::EncodableValue(flutter::EncodableMap{
      {flutter::EncodableValue(kEventType),
       flutter::EncodableValue("browserState")},
      {flutter::EncodableValue(kEventValue),
       flutter::EncodableValue(static_cast<int>(state))},
  });
  EmitEvent(event);
}

void BrowserBridge::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
{
  if (!browser_)
  {
    return result->Success();
  }
  if (method_call.method_name().compare("loadUrl") == 0)
  {
    if (const auto url = std::get_if<std::string>(method_call.arguments()))
    {
      CefString newUrl(*url);
      loadUrl(newUrl);
      return result->Success();
    }
  }
  if (method_call.method_name().compare("setToken") == 0)
  {
    if (const auto token = std::get_if<std::string>(method_call.arguments()))
    {
      setToken(*token);
      return result->Success();
    }
  }
  if (method_call.method_name().compare("setAccessToken") == 0)
  {
    if (const auto token = std::get_if<std::string>(method_call.arguments()))
    {
      setAccessToken(*token);
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("setSize") == 0)
  {
    const auto point = GetPointFromArgs(method_call.arguments());
    changeSize(point->first, point->second);
    result->Success();
  }
  else if (method_call.method_name().compare("setOffset") == 0)
  {
    const auto point = GetPointFromArgs(method_call.arguments());
    changeOffset(point->first, point->second);
    result->Success();
  }
  else if (method_call.method_name().compare("loadHTML") == 0)
  {
    if (const auto string = std::get_if<std::string>(method_call.arguments()))
    {
      loadHTML(*string);
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("clearAllCookies") == 0)
  {
    clearAllCookies();
    return result->Success();
  }
  else if (method_call.method_name().compare("getTextSelectionReport") == 0)
  {
    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(client::renderer::kTextSelectionReport);
    browser_->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
    return result->Success();
  }
  else if (method_call.method_name().compare("setZoomLevel") == 0)
  {
    if (const auto level = std::get_if<double>(method_call.arguments()))
    {
      setZoomLevel(*level);
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("getZoomLevel") == 0)
  {
    return result->Success(getZoomLevel());
  }
  else if (method_call.method_name().compare("executeJavaScript") == 0)
  {
    if (const auto js = std::get_if<std::string>(method_call.arguments()))
    {
      executeJavaScript(*js);
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("closeBrowser") == 0)
  {
    if (const auto force = std::get_if<bool>(method_call.arguments()))
    {
      closeBrowser(*force);
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("reload") == 0)
  {
    if (const auto ignoreCache = std::get_if<bool>(method_call.arguments()))
    {
      reload(*ignoreCache);
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("setHidden") == 0)
  {
    if (const auto hide = std::get_if<bool>(method_call.arguments()))
    {
      LOG(INFO) << "setting browser to hidden " << *hide;
      browser_->GetHost()->WasHidden(*hide);
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("setCurrent") == 0)
  {
    if (const auto current = std::get_if<bool>(method_call.arguments()))
    {
      isCurrent = *current;
      return result->Success();
    }
  }
  else if (method_call.method_name().compare("petTexture") == 0)
  {
    std::cout << "received request for " << texture_id_pet_ << " pt" << std::endl;
    return result->Success(flutter::EncodableValue(texture_id_pet_));
  }
  else if (method_call.method_name().compare("cursorClickDown") == 0)
  {
    const auto point = GetPointFromArgs(method_call.arguments());
    cursorClick(point->first, point->second, false);
    return result->Success();
  }
  else if (method_call.method_name().compare("contextMenu") == 0)
  {
  }
  else if (method_call.method_name().compare("setCursorPos") == 0)
  {
    const auto point = GetPointFromArgs(method_call.arguments());
    if (point)
    {
      setCursorPos(point->first, point->second);
    }
    return result->Success();
  }
  else if (method_call.method_name().compare("cursorClickUp") == 0)
  {
    const auto point = GetPointFromArgs(method_call.arguments());
    cursorClick(point->first, point->second, true);
    return result->Success();
  }
  else if (method_call.method_name().compare("setScrollDelta") == 0)
  {
    const auto point = GetPointFromArgs(method_call.arguments());
    if (point->second > 0)
    {
      scrollDown();
    }
    else if (point->second < 0)
    {
      scrollUp();
    }
    return result->Success();
  }
  else
  {
    return result->NotImplemented();
  }
}

void BrowserBridge::loadUrl(const CefString &url)
{
  browser_->GetMainFrame()->LoadURL(url);
}

void BrowserBridge::loadHTML(std::string text)
{
  browser_->GetMainFrame()->LoadURL(GetDataURI(text, "text/html"));
}

void BrowserBridge::scrollUp()
{
  CefMouseEvent ev;
  ev.x = 500;
  ev.y = 500;
  browser_->GetHost()->SendMouseWheelEvent(ev, 0, -100);
}

void BrowserBridge::scrollDown()
{
  CefMouseEvent ev;
  ev.x = 500;
  ev.y = 500;
  browser_->GetHost()->SendMouseWheelEvent(ev, 0, 100);
}

void BrowserBridge::changeSize(int w, int h)
{
  this->width = w;
  this->height = h;
  browser_->GetHost()->WasResized();
}

void BrowserBridge::changeOffset(int x, int y)
{
  this->current_offset_x = x;
  this->current_offset_y = y;
}

void BrowserBridge::reload(bool ignoreCache)
{
  if (ignoreCache)
  {
    browser_->ReloadIgnoreCache();
  }
  else
  {
    browser_->Reload();
  }
}

void BrowserBridge::setZoomLevel(double level)
{
  browser_->GetHost()->SetZoomLevel(level);
}

double BrowserBridge::getZoomLevel()
{
  return browser_->GetHost()->GetZoomLevel();
}

void BrowserBridge::executeJavaScript(std::string js)
{
  browser_->GetMainFrame()->ExecuteJavaScript(js, browser_->GetMainFrame()->GetURL(), 0);
}

void BrowserBridge::cursorClick(int x, int y, bool up)
{
  CefMouseEvent ev;
  ev.x = x;
  ev.y = y;
  browser_->GetHost()->SetFocus(true);
  browser_->GetHost()->SendMouseClickEvent(ev, CefBrowserHost::MouseButtonType::MBT_LEFT, up, 1);
}

void BrowserBridge::sendKeyEvent(const CefKeyEvent &event)
{
  browser_->GetHost()->SendKeyEvent(event);
}

void BrowserBridge::sendMouseWheelEvent(CefMouseEvent &event,
                                        int deltaX,
                                        int deltaY)
{
  event.x = event.x - current_offset_x;
  event.y = event.y - current_offset_y;
  browser_->GetHost()->SendMouseWheelEvent(event, deltaX, deltaY);
}

void BrowserBridge::sendMouseClickEvent(CefMouseEvent &event,
                                        CefBrowserHost::MouseButtonType type,
                                        bool mouseUp,
                                        int clickCount)
{
  event.x = event.x - current_offset_x;
  event.y = event.y - current_offset_y;

  if (type == MBT_RIGHT && mouseUp == false)
  {
    const auto context_menu_event = flutter::EncodableValue(flutter::EncodableMap{
        {flutter::EncodableValue(kEventType),
         flutter::EncodableValue("showContextMenu")},
    });
    EmitEvent(context_menu_event);
  }
  browser_->GetHost()->SetFocus(true);
  browser_->GetHost()->SendMouseClickEvent(event, type, mouseUp, clickCount);
}

void BrowserBridge::sendMouseMoveEvent(CefMouseEvent &event,
                                       bool mouseLeave)
{

  event.x = event.x - current_offset_x;
  event.y = event.y - current_offset_y;
  browser_->GetHost()->SendMouseMoveEvent(event, false);
}

void BrowserBridge::setCursorPos(int x, int y)
{
  CefMouseEvent ev;
  ev.x = x;
  ev.y = y;
  browser_->GetHost()->SendMouseMoveEvent(ev, false);
}

void BrowserBridge::closeBrowser(bool force)
{
  closing = true;
  browser_->GetHost()->CloseBrowser(force);
}

void BrowserBridge::resetBrowser()
{
  browser_.reset();
}