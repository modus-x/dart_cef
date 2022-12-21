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
#include "dart_cef_plugin.h"

namespace
{
  SimpleHandler *g_instance = nullptr;

} // namespace

SimpleHandler::SimpleHandler()
    : is_closing_(false)
{
  g_instance = this;
  dart_cef::DartCefPlugin::handler = this;
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
  auto bridge = getBridge(browser->GetIdentifier());

  if (bridge)
  {
    bridge->closing = true;
    cache_.erase(browser->GetIdentifier());
    browser_list_[bridge->texture_id()]->resetBrowser();
    LOG(INFO) << "CEFSHUTDOWN erased from cache_, browserbridge's browser reset, sending event...";
    bridge->OnShutdown();
  }
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
    value->browser_->StopLoad();
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

bool SimpleHandler::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> dragData,
                                CefDragHandler::DragOperationsMask mask)
{
  CEF_REQUIRE_UI_THREAD();
  return false;
}

void SimpleHandler::OnDraggableRegionsChanged(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const std::vector<CefDraggableRegion> &regions)
{
  CEF_REQUIRE_UI_THREAD();
  NotifyDraggableRegions(regions);
}

bool SimpleHandler::StartDragging(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefDragData> drag_data,
                                  CefDragHandler::DragOperationsMask allowed_ops,
                                  int x,
                                  int y)
{
  LOG(INFO) << "StartDragging " << browser->GetIdentifier();

  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge)
  {
    return bridge->StartDragging(drag_data, allowed_ops, x, y);
  }
  return false;
}

void SimpleHandler::NotifyDraggableRegions(
    const std::vector<CefDraggableRegion> &regions)
{
  // if (!CURRENTLY_ON_MAIN_THREAD()) {
  // Execute this method on the main thread.
  //   MAIN_POST_CLOSURE(
  //       base::BindOnce(&ClientHandler::NotifyDraggableRegions, this, regions));
  //   return;
  // }

  // if (delegate_)
  //   delegate_->OnSetDraggableRegions(regions);
}

int64_t SimpleHandler::createBrowser(flutter::BinaryMessenger *messenger,
                                     flutter::TextureRegistrar *texture_registrar, const CefString &url, const CefString &bind_func, const CefString &token, const CefString &access_token)
{
  CefRefPtr<BrowserBridge> bridge(new BrowserBridge(messenger, texture_registrar, url, bind_func, token, access_token));
  auto texture_id = bridge->texture_id();
  browser_list_[texture_id] = bridge;
  return texture_id;
}

void SimpleHandler::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList &dirtyRects, const void *buffer, int w, int h)
{
  CEF_REQUIRE_UI_THREAD();
  auto bridge = getBridge(browser->GetIdentifier());
  if (bridge && !bridge->closing && bridge->texture_bridge && !bridge->closing)
  {
    if (type == PET_POPUP)
    {
      bridge->texture_bridge_pet->sendBuffer(buffer, w, h);
    }
    else
    {
      bridge->texture_bridge->sendBuffer(buffer, w, h);
    }
  }
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
