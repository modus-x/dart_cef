#include "client_browser.h"

#include "browser_delegate.h"

namespace client {

// static
void ClientAppBrowser::CreateDelegates(DelegateSet& delegates) {
  browser::CreateDelegates(delegates);
}

}  // namespace client
