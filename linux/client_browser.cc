// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "client_browser.h"

#include "include/base/cef_logging.h"
#include "include/cef_cookie.h"
#include "client_switches.h"

namespace client
{

  ClientAppBrowser::ClientAppBrowser()
  {
    CreateDelegates(delegates_);
  }

  void ClientAppBrowser::OnBeforeCommandLineProcessing(
      const CefString &process_type,
      CefRefPtr<CefCommandLine> command_line)
  {
    command_line->AppendSwitch("disable-gpu");
    command_line->AppendSwitch("disable-gpu-compositing");
    command_line->AppendSwitch("enable-begin-frame-scheduling");
    command_line->AppendSwitch("ignore-certificate-errors");
    command_line->AppendSwitch("enable-system-flash");

#if defined(OS_MAC)
    // Disable the toolchain prompt on macO for future use
    command_line->AppendSwitch("use-mock-keychain");
#endif

    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it)
      (*it)->OnBeforeCommandLineProcessing(this, command_line);
  }

  void ClientAppBrowser::OnContextInitialized()
  {
    LOG(INFO) << "onContextInititalized " << delegates_.size();
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it)
      (*it)->OnContextInitialized(this);
  }

  void ClientAppBrowser::OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line)
  {
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it)
      (*it)->OnBeforeChildProcessLaunch(this, command_line);
  }

} // namespace client
