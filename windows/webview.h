#pragma once

constexpr auto kEventType = "type";
constexpr auto kEventValue = "value";
constexpr auto kErrorInvalidArgs = "invalidArguments";

enum class WebviewLoadingState
{
  InProcess,
  NavigationCompleted
};

enum class WebviewState
{
  Loading,
  Ready,
  Error,
  Shutdown,
};

enum class WebviewEvent
{
  JsContextCreated,
  CookiesCleared,
  TokenUpdated,
  AccessTokenUpdated
};

