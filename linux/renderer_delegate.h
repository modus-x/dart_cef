// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_RENDERER_CLIENT_RENDERER_H_
#define CEF_TESTS_CEFCLIENT_RENDERER_CLIENT_RENDERER_H_
#pragma once

#include "include/cef_base.h"
#include "client_renderer.h"
#include <functional>

namespace client
{
    namespace renderer
    {
        typedef std::function<bool(const CefString &)> WebMessageCallback;

        const char kFocusedNodeChangedMessage[] = "ClientRenderer.FocusedNodeChanged";
        const char kTextSelectionReport[] = "ClientRenderer.TextSelectionReport";
        const char kBrowserCreatedMessage[] = "ClientRenderer.BrowserCreated";
        const char kWebMessage[] = "ClientRenderer.WebMessage";
        const char kContextCreated[] = "ClientRenderer.ContextCreated";
        const char kTokenUpdate[] = "ClientRenderer.TokenUpdate";
        const char kAccessTokenUpdate[] = "ClientRenderer.AccessTokenUpdate";

        // Create the renderer delegate. Called from client_app_delegates_renderer.cc.
        void CreateDelegates(ClientAppRenderer::DelegateSet &delegates);

    } // namespace renderer
} // namespace client

#endif // CEF_TESTS_CEFCLIENT_RENDERER_CLIENT_RENDERER_H_
