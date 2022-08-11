enum LoadingState { inProcess, navigationCompleted }

enum WebviewState {
  loading,
  ready,
  error,
  shutdown
}

enum WebviewEvent {
  jsContextCreated,
  cookiesCleared,
  tokenUpdated,
  accessTokenUpdated
}