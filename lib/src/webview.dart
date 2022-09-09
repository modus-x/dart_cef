import 'dart:async';
import 'dart:convert';

import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';
import 'enums.dart';
import 'package:context_menus/context_menus.dart';

import 'cursor.dart';

class CommonContextMenu extends StatefulWidget {
  const CommonContextMenu({Key? key, required this.controller})
      : super(key: key);

  final WebviewController controller;

  @override
  State<CommonContextMenu> createState() => _CommonContextMenuState();
}

class _CommonContextMenuState extends State<CommonContextMenu>
    with ContextMenuStateMixin {
  @override
  Widget build(BuildContext context) {
    // cardBuilder is provided to us by the mixin, pass it a list of children to layout
    return cardBuilder.call(
      context,
      [
        buttonBuilder.call(
          context,
          ContextMenuButtonConfig(
            "Копировать",
            icon: const Icon(Icons.copy, size: 18),
            onPressed: () => handlePressed(context, () {
              widget.controller.getTextSelectionReport();
            }),
          ),
        )
      ],
    );
  }
}

extension GlobalKeyExtension on GlobalKey {
  Rect? get globalPaintBounds {
    final renderObject = currentContext?.findRenderObject();
    final translation = renderObject?.getTransformTo(null).getTranslation();
    if (translation != null && renderObject?.paintBounds != null) {
      final offset = Offset(translation.x, translation.y);
      return renderObject!.paintBounds.shift(offset);
    } else {
      return null;
    }
  }
}

class WidgetSizeRenderObject extends RenderProxyBox {
  final void Function(Size size) onSizeChange;
  final void Function(Offset position) onPositionChange;

  WidgetSizeRenderObject(this.onSizeChange, this.onPositionChange);

  @override
  void performLayout() {
    try {
      final BoxConstraints constraints = this.constraints;
      child!.layout(constraints, parentUsesSize: true);
      size = constraints.constrain(Size(
        child!.size.width,
        child!.size.height,
      ));
      WidgetsBinding.instance.addPostFrameCallback((_) {
        onSizeChange(child!.size);
      });
      WidgetsBinding.instance.addPostFrameCallback((_) {
        onPositionChange(child!.localToGlobal(Offset.zero));
      });
    } catch (e) {
      print("error is $e");
    }
  }
}

class BrowserSizeOffsetWrapper extends SingleChildRenderObjectWidget {
  final void Function(Size size) onSizeChange;
  final void Function(Offset position) onPositionChange;

  const BrowserSizeOffsetWrapper({
    Key? key,
    required this.onSizeChange,
    required this.onPositionChange,
    required Widget child,
  }) : super(key: key, child: child);

  @override
  RenderObject createRenderObject(BuildContext context) {
    return WidgetSizeRenderObject(onSizeChange, onPositionChange);
  }
}

const String _pluginChannelPrefix = 'webview_cef';

const MethodChannel _pluginMethodChannel = MethodChannel(_pluginChannelPrefix);
const EventChannel _pluginEventChannel =
    EventChannel("${_pluginChannelPrefix}_events");

Completer<void>? _shuttingDownCompleter;

int activeBrowsers = 0;

Future<void> shudownAllCEFBrowsers(bool force) async {
  return _pluginMethodChannel.invokeMethod('closeAllBrowsers', force);
}

Future<void> shutdownCEF() async {
  if (activeBrowsers > 0) {
    _shuttingDownCompleter = Completer();
    await shudownAllCEFBrowsers(true);
    await _shuttingDownCompleter!.future;
    await _pluginMethodChannel.invokeMethod("shutdown");
  }
}

class CefRect {
  int x;
  int y;
  int width;
  int height;

  CefRect(
      {required this.x,
      required this.y,
      required this.width,
      required this.height});
}

class WebviewController extends ValueNotifier<bool> {
  late Completer<void> _creatingCompleter;

  late MethodChannel _methodChannel;
  late EventChannel _eventChannel;

  int _textureId = 0;
  int _petTextureId = 0;
  bool _isDisposed = false;

  // key for manual updates of offset, e.g. after animations
  final key = GlobalKey();

  final StreamController<String> _urlStreamController =
      StreamController<String>.broadcast();
  Stream<String> get url => _urlStreamController.stream;

  final StreamController<bool> _contextMenuShowController =
      StreamController<bool>.broadcast();
  Stream<bool> get showContextMenu => _contextMenuShowController.stream;

  final StreamController<bool> _popShowController =
      StreamController<bool>.broadcast();
  Stream<bool> get showPopup => _popShowController.stream;

  final StreamController<CefRect?> _popRectController =
      StreamController<CefRect?>.broadcast();
  Stream<CefRect?> get popupRect => _popRectController.stream;

  final StreamController<String> _titleStreamController =
      StreamController<String>.broadcast();
  Stream<String> get title => _titleStreamController.stream;

  final StreamController<String> _textSelectionController =
      StreamController<String>.broadcast();
  Stream<String> get textSelection => _textSelectionController.stream;

  final StreamController<SystemMouseCursor> _cursorStreamController =
      StreamController<SystemMouseCursor>.broadcast();
  Stream<SystemMouseCursor> get _cursor => _cursorStreamController.stream;

  final StreamController<LoadingState> _loadingStateStreamController =
      StreamController<LoadingState>.broadcast();
  Stream<LoadingState> get loadingState => _loadingStateStreamController.stream;

  final StreamController<WebviewState> _webviewStateStreamController =
      StreamController<WebviewState>.broadcast();
  Stream<WebviewState> get webviewState => _webviewStateStreamController.stream;

  final StreamController<Map<dynamic, dynamic>> _webMessageStreamController =
      StreamController<Map<dynamic, dynamic>>.broadcast();
  Stream<Map<dynamic, dynamic>> get webMessage =>
      _webMessageStreamController.stream;

  final StreamController<WebviewEvent> _browserEventsController =
      StreamController<WebviewEvent>.broadcast();
  Stream<WebviewEvent> get browserEvents => _browserEventsController.stream;

  WebviewController() : super(false);

  Future<void> get ready => _creatingCompleter.future;

  /// Initializes the underlying platform view.
  Future<void> initialize(
      {String startUrl = "about:blank",
      String webMessageFunction = "postMessage",
      bool isHTML = false,
      String token = "",
      String accessToken = ""}) async {
    if (_isDisposed || value) {
      return Future<void>.value();
    }
    _creatingCompleter = Completer<void>();
    try {
      _textureId = await _pluginMethodChannel
              .invokeMethod<int>('createBrowser', <String, dynamic>{
            'startUrl': startUrl,
            'webMessageFunction': webMessageFunction,
            'isHTML': isHTML,
            'token': token,
            'accessToken': accessToken
          }) ??
          0;
      _methodChannel = MethodChannel('$_pluginChannelPrefix/$_textureId');
      _eventChannel = EventChannel('$_pluginChannelPrefix/$_textureId/events');
      _eventChannel.receiveBroadcastStream().listen((event) async {
        final map = event as Map<dynamic, dynamic>;
        switch (map['type']) {
          case 'browserEvent':
            final event = WebviewEvent.values[map['value']];
            print("webview_cef recieved browserEvent $event");
            _browserEventsController.add(event);
            break;
          case 'loadingState':
            final state = LoadingState.values[map['value']];
            _loadingStateStreamController.add(state);
            break;
          case 'textSelectionReport':
            _textSelectionController.add(map["value"]);
            break;
          case "browserState":
            final state = WebviewState.values[map['value']];
            if (state == WebviewState.ready &&
                !_creatingCompleter.isCompleted) {
              _petTextureId =
                  await _methodChannel.invokeMethod<int>('petTexture') ?? 0;
              _creatingCompleter.complete();
              value = true;
              activeBrowsers++;
            }
            if (state == WebviewState.shutdown) {
              activeBrowsers--;
              if (_shuttingDownCompleter != null && activeBrowsers == 0) {
                _shuttingDownCompleter!.complete();
              }
            }
            _webviewStateStreamController.add(state);
            break;
          case "urlChanged":
            _urlStreamController.add(map["value"]);
            break;
          case "popupShow":
            if (map["value"] == false) {
              _popRectController.add(null);
            }
            _popShowController.add(map["value"]);
            break;
          case "popupSize":
            _popRectController.add(CefRect(
                x: map["x"],
                y: map["y"],
                width: map["width"],
                height: map["height"]));
            break;
          case 'titleChanged':
            _titleStreamController.add(map['value']);
            break;
          case 'cursorChanged':
            _cursorStreamController.add(getCursorByName(map['value']));
            break;
          case 'showContextMenu':
            _contextMenuShowController.add(true);
            break;
          case 'webMessage':
            try {
              final message = json.decode(map['value']);
              _webMessageStreamController.add(message);
            } catch (ex) {
              _webMessageStreamController.addError(ex);
            }
        }
      });
    } on PlatformException catch (e) {
      _creatingCompleter.completeError(e);
    }

    value = true;
    return _creatingCompleter.future;
  }

  @override
  Future<void> dispose() async {
    await _creatingCompleter.future;
    if (!_isDisposed) {
      _isDisposed = true;
      await _methodChannel.invokeMethod('closeBrowser', true);
    }
    super.dispose();
  }

  /// Loads the given [url].
  Future<void> loadUrl(String url) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('loadUrl', url);
  }

  Future<void> loadHTML(String text) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('loadHTML', text);
  }

  Future<void> setZoomLevel(double level) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('setZoomLevel', level);
  }

  Future<void> getTextSelectionReport() async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('getTextSelectionReport');
  }

  Future<void> clearAllCookies() async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('clearAllCookies');
  }

  Future<double> getZoomLevel() async {
    if (_isDisposed) {
      return 0;
    }
    assert(value);
    final zoom = await _methodChannel.invokeMethod('getZoomLevel');
    return zoom;
  }

  Future<void> executeJavaScript(String js) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('executeJavaScript', js);
  }

  Future<void> setToken(String token) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('setToken', token);
  }

  Future<void> setAccessToken(String token) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('setAccessToken', token);
  }

  Future<void> reload(bool ignoreCache) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('reload', ignoreCache);
  }

  Future<void> setHidden(bool hidden) async {
    if (_isDisposed) {
      return;
    }
    if (!value) {
      return;
    }
    return _methodChannel.invokeMethod('setHidden', hidden);
  }

  Future<void> setCurrent(bool current) async {
    if (_isDisposed) {
      return;
    }
    if (!value) {
      return;
    }
    return _methodChannel.invokeMethod('setCurrent', current);
  }

  /// Sets the surface size to the provided [size].
  Future<void> _setSize(Size size) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('setSize',
        {"width": size.width.round(), "height": size.height.round()});
  }

  Future<void> _setOffset(Offset offset) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod(
        'setOffset', {"x": offset.dx.round(), "y": offset.dy.round()});
  }

  Future<void> _setScrollDelta(int dx, int dy) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('setScrollDelta', {"x": dx, "y": dy});
  }

  Future<void> updateOffset() async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    var bounds = key.globalPaintBounds;
    var offset = bounds?.topLeft;
    if (offset != null) {
      return _methodChannel
          .invokeMethod('setOffset', [offset.dx.round(), offset.dy.round()]);
    }
  }

  Future<void> _cursorClickDown(Offset position) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod('cursorClickDown',
        {"x": position.dx.round(), "y": position.dy.round()});
  }

  Future<void> _cursorClickUp(Offset position) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod(
        'cursorClickUp', {"x": position.dx.round(), "y": position.dy.round()});
  }

  Future<void> _setCursorPos(Offset position) async {
    if (_isDisposed) {
      return;
    }
    assert(value);
    return _methodChannel.invokeMethod(
        'setCursorPos', {"x": position.dx.round(), "y": position.dy.round()});
  }
}

class Webview extends StatefulWidget {
  final WebviewController controller;

  const Webview(this.controller, {Key? key}) : super(key: key);

  @override
  _WebviewState createState() => _WebviewState();
}

class _WebviewState extends State<Webview> {
  final FocusNode webViewFocus = FocusNode();

  WebviewController get _controller => widget.controller;
  MouseCursor _cursor = SystemMouseCursors.basic;

  StreamSubscription? _cursorSubscription;
  StreamSubscription? _textSelectionSubscription;

  var current = false;

  @override
  void initState() {
    super.initState();
    _cursorSubscription = _controller._cursor.listen((cursor) {
      setState(() {
        _cursor = cursor;
      });
    });
    _textSelectionSubscription = _controller.textSelection.listen((event) {
      Clipboard.setData(ClipboardData(text: event));
    });
    _controller.setHidden(false);
  }

  @override
  void dispose() async {
    _cursorSubscription?.cancel();
    _textSelectionSubscription?.cancel();
    super.dispose();
    _controller.setHidden(true);
    _controller.setCurrent(false);
  }

  @override
  Widget build(BuildContext context) {
    return SizedBox.expand(
      child: Focus(
        child: _buildInner(),
        focusNode: webViewFocus,
      ),
    );
  }

  Widget _buildInner() {
    return ContextMenuOverlay(
      child: ContextMenuRegion(
          enableLongPress: false,
          contextMenu: CommonContextMenu(
            controller: widget.controller,
          ),
          child: Listener(
            onPointerSignal: (signal) {
              if (signal is PointerScrollEvent) {
                _controller._setScrollDelta(-signal.scrollDelta.dx.round(),
                    -signal.scrollDelta.dy.round());
              }
            },
            onPointerDown: (ev) {
              _controller._cursorClickDown(ev.localPosition);
            },
            onPointerUp: (ev) {
              _controller._cursorClickUp(ev.localPosition);
            },
            onPointerHover: (ev) {
              _controller._setCursorPos(ev.localPosition);
              if (!webViewFocus.hasFocus) {
                webViewFocus.requestFocus();
              }
              if (!current) {
                setState(() {
                  current = true;
                  _controller.setCurrent(true);
                });
              }
            },
            child: MouseRegion(
                onEnter: (event) {
                  if (!webViewFocus.hasFocus) {
                    webViewFocus.requestFocus();
                  }
                  if (!current) {
                    setState(() {
                      _controller.setCurrent(true);
                      current = true;
                    });
                  }
                },
                onExit: (event) {
                  if (webViewFocus.hasFocus) {
                    webViewFocus.unfocus();
                  }
                  if (current) {
                    setState(() {
                      _controller.setCurrent(false);
                      current = false;
                    });
                  }
                },
                cursor: _cursor,
                child: WebviewCore(controller: _controller)),
          )),
    );
  }
}

class WebviewCore extends StatefulWidget {
  const WebviewCore({
    Key? key,
    required WebviewController controller,
  })  : _controller = controller,
        super(key: key);

  final WebviewController _controller;

  @override
  State<WebviewCore> createState() => _WebviewCoreState();
}

class _WebviewCoreState extends State<WebviewCore> {
  late StreamSubscription<bool> sub;
  bool isShown = false;

  @override
  void initState() {
    super.initState();

    sub = widget._controller.showContextMenu.listen((event) {
      if (event && !context.contextMenuOverlay.mounted) {
        context.contextMenuOverlay.show(CommonContextMenu(
          controller: widget._controller,
        ));
        setState(() {
          isShown = true;
        });
      } else if (!event && context.contextMenuOverlay.mounted) {
        context.contextMenuOverlay.close();
        setState(() {
          isShown = false;
        });
      }
    });
  }

  @override
  void dispose() {
    super.dispose();
    sub.cancel();
  }

  @override
  Widget build(BuildContext context) {
    return BrowserSizeOffsetWrapper(
      onSizeChange: ((size) async {
        await widget._controller.ready;
        if (size.width == 0 || size.height == 0) {
          return;
        }
        unawaited(widget._controller._setSize(size));
      }),
      onPositionChange: (Offset position) async {
        await widget._controller.ready;
        unawaited(widget._controller._setOffset(position));
      },
      child: Stack(
        key: widget._controller.key,
        children: [
          Texture(
            textureId: widget._controller._textureId,
          ),
          StreamBuilder<bool>(
              stream: widget._controller.showPopup,
              builder: (context, showPopup) {
                return StreamBuilder<CefRect?>(
                    stream: widget._controller.popupRect,
                    builder: (context, rect) {
                      if (rect.hasData && rect.data is CefRect) {
                        return Positioned(
                            left: rect.data?.x.toDouble(),
                            top: rect.data?.y.toDouble(),
                            child: SizedBox(
                                width: rect.data?.width.toDouble(),
                                height: rect.data?.height.toDouble(),
                                child: Texture(
                                  textureId: widget._controller._petTextureId,
                                )));
                      }

                      return Container();
                    });
              })
        ],
      ),
    );
  }
}
