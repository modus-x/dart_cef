
#include "renderer_delegate.h"
#include "client_renderer.h"

namespace client {

// static
void ClientAppRenderer::CreateDelegates(DelegateSet& delegates) {
  renderer::CreateDelegates(delegates);
}

}  // namespace client
