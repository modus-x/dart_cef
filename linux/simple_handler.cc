// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
#include "simple_handler.h"

#include <sstream>
#include <string>
#include <iostream>

#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#include "renderer_delegate.h"
#include "data.h"
#include "webview.h"
#include <fmt/core.h>
#include "simple_handler.h"

namespace
{
  SimpleHandler *g_instance = nullptr;

} // namespace

SimpleHandler::SimpleHandler()
    : is_closing_(false)
{
  g_instance = this;
}

SimpleHandler::~SimpleHandler()
{
  g_instance = nullptr;
}

// static
SimpleHandler *SimpleHandler::GetInstance()
{
  return g_instance;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();
  LOG(INFO) << "OnAfterCreated for browser " << browser->GetIdentifier();
}

bool SimpleHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
  // Check the message name.
  const std::string &message_name = message->GetName();
  LOG(INFO) << "recieved " << message_name << " message";
  if (message_name == client::renderer::kBrowserCreatedMessage)
  {
    int id = browser->GetIdentifier();
    int64_t texture_id = std::stoll(message->GetArgumentList()->GetString(0).ToString(), NULL, 10);
    LOG(INFO) << "OnBrowserCreated for browser " << browser->GetIdentifier() << " with texture " << texture_id;
    cache_[id] = texture_id;
    browser_list_[texture_id]->setBrowser(browser);
    browser_list_[texture_id]->OnAfterCreated();
    return true;
  }
  else if (message_name == client::renderer::kWebMessage)
  {
    CefString webMessage = message->GetArgumentList()->GetString(0).ToString();
    auto bridge = getBridge(browser->GetIdentifier());
    if (bridge)
    {
      bridge->OnWebMessage(webMessage);
    }
    return true;
  }
  else if (message_name == client::renderer::kContextCreated)
  {
    auto bridge = getBridge(browser->GetIdentifier());
    if (bridge)
    {
      bridge->browserEvent(WebviewEvent::JsContextCreated);
    }
  }
  else if (message_name == client::renderer::kTokenUpdate)
  {
    auto bridge = getBridge(browser->GetIdentifier());
    if (bridge)
    {
      bridge->browserEvent(WebviewEvent::TokenUpdated);
    }
  }
  else if (message_name == client::renderer::kAccessTokenUpdate)
  {
    auto bridge = getBridge(browser->GetIdentifier());
    if (bridge)
    {
      bridge->browserEvent(WebviewEvent::AccessTokenUpdated);
    }
  }
  else if (message_name == client::renderer::kTextSelectionReport)
  {
    CefString text = message->GetArgumentList()->GetString(0).ToString();
    auto bridge = getBridge(browser->GetIdentifier());
    if (bridge)
    {
      bridge->textSelectionReport(text);
    }
  }
  return false;
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();
  // auto bridge = getBridge(browser->GetIdentifier());

  // if (bridge)
  // {
  //   cache_.erase(browser->GetIdentifier());
  //   auto video_outlet_private =
  //         (VideoOutletPrivate*)video_outlet_get_instance_private(bridge->texture_bridge);
  //   browser_list_[video_outlet_private->texture_id]->resetBrowser();
  //   bridge->OnShutdown();
  // }
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString &errorText,
                                const CefString &failedUrl)
{
  CEF_REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Display a load error message using a data: URI.
  std::stringstream ss;
  ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
     << std::string(failedUrl) << " with error " << std::string(errorText)
     << " (" << errorCode << ").</h2></body></html>";

  frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void SimpleHandler::CloseAllBrowsers(bool force_close)
{
  if (browser_list_.empty())
    return;
  if (!CefCurrentlyOn(TID_UI))
  {
    CefPostTask(TID_UI, base::BindOnce(&SimpleHandler::CloseAllBrowsers, this,
                                       force_close));
    return;
  }

  for (const auto &[key, value] : browser_list_)
  {

    LOG(INFO) << "closing browser with texture " << key;
    value->closeBrowser(force_close);
  }
}

void SimpleHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward)
{
  CEF_REQUIRE_UI_THREAD();
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    bridge->OnLoadingStateChange(isLoading, canGoBack, canGoForward);
  }
}

void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode){};

void SimpleHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString &url)
{
  CEF_REQUIRE_UI_THREAD();
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    bridge->OnUrlChanged(url);
  }
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString &title)
{
  CEF_REQUIRE_UI_THREAD();
  LOG(INFO) << "onTitleChange: " << title << " for browser " << browser->GetIdentifier();
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    bridge->OnTitleChanged(title);
  }
}

bool SimpleHandler::OnCursorChange(CefRefPtr<CefBrowser> browser,
                                   CefCursorHandle cursor,
                                   cef_cursor_type_t type,
                                   const CefCursorInfo &custom_cursor_info)
{
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    bridge->onCursorChanged(type);
    return true;
  }
  return false;
}

void SimpleHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
  rect.x = rect.y = 0;
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    rect.width = bridge->width;
    rect.height = bridge->height;
  }
  else
  {
    rect.width = 1920;
    rect.height = 1080;
  }
  return;
}

int64_t SimpleHandler::createBrowser(
    FlBinaryMessenger *messenger,
    FlTextureRegistrar *texture_registrar, const CefString &url, const CefString &bind_func, const CefString &token, const CefString &access_token)
{
  // CefRefPtr<BrowserBridge> bridge(new BrowserBridge(messenger, texture_registrar, url, bind_func, token, access_token));
  // auto texture_id = bridge->texture_id();
  // browser_list_[texture_id] = bridge;
  return 0;
}

void SimpleHandler::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList &dirtyRects, const void *buffer, int w, int h)
{
  CEF_REQUIRE_UI_THREAD();
  auto bridge = getBridge(browser->GetIdentifier());
  // if (bridge && !bridge->closing && bridge->texture_bridge)
  // {
  //   if (type == PET_POPUP)
  //   {
  //   }
  //   else
  //   {
  //   }
  // }
}

void SimpleHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    bridge->onPopupShow(show);
  }
}

void SimpleHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect)
{
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    bridge->OnPopupSize(rect.x, rect.y, rect.width, rect.height);
  }
}

CefRefPtr<BrowserBridge> SimpleHandler::getBridge(int browser_id)
{
  if (cache_.count(browser_id))
  {
    int64_t texture_id = cache_[browser_id];
    if (browser_list_.count(texture_id))
    {
      return browser_list_[texture_id];
    }
  }
  return nullptr;
}

void SimpleHandler::sendKeyEvent(const CefKeyEvent &event)
{
  for (auto const &[key, val] : browser_list_)
  {
    if (val->isCurrent)
    {
      val->sendKeyEvent(event);
      break;
    }
  }
}

void SimpleHandler::sendMouseWheelEvent(CefMouseEvent &event,
                                        int deltaX,
                                        int deltaY)
{
  for (auto const &[key, val] : browser_list_)
  {
    if (val->isCurrent)
    {
      val->sendMouseWheelEvent(event, deltaX, deltaY);
      break;
    }
  }
}

void SimpleHandler::sendMouseClickEvent(CefMouseEvent &event,
                                        CefBrowserHost::MouseButtonType type,
                                        bool mouseUp,
                                        int clickCount)
{
  for (auto const &[key, val] : browser_list_)
  {
    if (val->isCurrent)
    {
      val->sendMouseClickEvent(event, type, mouseUp, clickCount);
      break;
    }
  }
}

void SimpleHandler::sendMouseMoveEvent(CefMouseEvent &event,
                                       bool mouseLeave)
{
  for (auto const &[key, val] : browser_list_)
  {
    if (val->isCurrent)
    {
      val->sendMouseMoveEvent(event, mouseLeave);
      break;
    }
  }
}
