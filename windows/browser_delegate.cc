// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "browser_delegate.h"

#include "include/cef_command_line.h"
#include "include/cef_crash_util.h"
#include "include/cef_file_util.h"
#include "client_switches.h"

namespace client
{
    namespace browser
    {

        namespace
        {

            class ClientBrowserDelegate : public ClientAppBrowser::Delegate
            {
            public:
                ClientBrowserDelegate() {}

                void OnContextInitialized(CefRefPtr<ClientAppBrowser> app) override
                {
                }

                void OnBeforeCommandLineProcessing(
                    CefRefPtr<ClientAppBrowser> app,
                    CefRefPtr<CefCommandLine> command_line) override
                {
                }

            private:
                DISALLOW_COPY_AND_ASSIGN(ClientBrowserDelegate);
                IMPLEMENT_REFCOUNTING(ClientBrowserDelegate);
            };

        } // namespace

        void CreateDelegates(ClientAppBrowser::DelegateSet &delegates)
        {
            delegates.insert(new ClientBrowserDelegate);
        }

    } // namespace browser
} // namespace client