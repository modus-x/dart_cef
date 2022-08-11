import 'dart:convert';
import 'dart:math';

import 'package:flutter/material.dart';
import 'dart:async';

import 'package:webview_cef/webview_cef.dart';
import 'package:window_manager/window_manager.dart';

double dp(double val, int places) {
  num mod = pow(10.0, places);
  return ((val * mod).round().toDouble() / mod);
}

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp>
    with WindowListener, SingleTickerProviderStateMixin {
  late TabController _tabController;

  final googleController = WebviewController();
  final bingController = WebviewController();

  @override
  void initState() {
    windowManager.addListener(this);
    _init();
    super.initState();
    _tabController = TabController(vsync: this, length: 2);
    _tabController.animation?.addStatusListener((status) {
      if (status == AnimationStatus.completed) {
        if (_tabController.index == 0) {
          googleController.updateOffset();
        } else {
          bingController.updateOffset();
        }
      }
    });
  }

  @override
  void onWindowClose() async {
    await shutdownCEF();
    windowManager.destroy();
  }

  void _init() async {
    await windowManager.setPreventClose(true);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          bottom:  TabBar(
            controller: _tabController,
            tabs: const [
              Tab(text: "First browser"),
              Tab(text: "Second browser"),
            ],
          ),
          title: const Text('Browser Demo'),
        ),
        body: TabBarView(
          controller: _tabController,
          children: [
            WebviewBar(
              url: "https://google.com",
              webviewController: googleController,
            ),
            WebviewBar(
              url: "https://bing.com",
              webviewController: bingController,
            ),
          ],
        ),
      ),
    );
  }
}

class WebviewBar extends StatefulWidget {
  WebviewBar({Key? key, required this.url, required this.webviewController})
      : super(key: key);

  String url;
  WebviewController webviewController;

  @override
  State<WebviewBar> createState() => _WebviewBarState();
}

// It's your choice how to store controllers. You may use providers / keep contfollers alive only when widget
// is built and make these controllers destroy CEF browsers on widget dispose
class _WebviewBarState extends State<WebviewBar>
    with AutomaticKeepAliveClientMixin {
  final _textController = TextEditingController();

  double zoom = 0;

  @override
  void initState() {
    super.initState();
    initPlatformState();
  }

  // Platform messages are asynchronous, so we initialize in an async method.
  Future<void> initPlatformState() async {
    _textController.text = widget.url;
    await widget.webviewController
        .initialize(startUrl: widget.url, webMessageFunction: "postMessage");
    widget.webviewController.url.listen((url) {
      _textController.text = url;
    });
    widget.webviewController.webMessage.listen((message) {
      showAlert(message);
    });
    // zoom = await _controller.getZoomLevel(); - CALLED ON WRONG THREAD, need to handle this

    // If the widget was removed from the tree while the asynchronous platform
    // message was in flight, we want to discard the reply rather than calling
    // setState to update our non-existent appearance.
    if (!mounted) return;

    setState(() {});
  }

  void showAlert(Map<dynamic, dynamic> event) {
    showDialog(
        context: context,
        builder: (context) => AlertDialog(
              content: Text(
                  "Event type: ${event['type']}, created on: ${event['timestamp']}"),
              actions: <Widget>[
                TextButton(
                  onPressed: () => Navigator.pop(context, 'Cancel'),
                  child: const Text('Cancel'),
                ),
              ],
            ));
  }

  @override
  Widget build(BuildContext context) {
    return compositeView();
  }

  Widget compositeView() {
    if (!widget.webviewController.value) {
      return const Text(
        'Not Initialized, awaiting',
        style: TextStyle(
          fontSize: 24.0,
          fontWeight: FontWeight.w900,
        ),
      );
    } else {
      return Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            Card(
              elevation: 0,
              child: Row(children: [
                Expanded(
                  child: TextField(
                    decoration: const InputDecoration(
                      hintText: 'URL',
                      contentPadding: EdgeInsets.all(10.0),
                    ),
                    textAlignVertical: TextAlignVertical.center,
                    controller: _textController,
                    onSubmitted: (val) {
                      print("webview_cef loading $val");
                      widget.webviewController.loadUrl(val);
                    },
                  ),
                ),
                IconButton(
                  icon: const Icon(Icons.refresh),
                  splashRadius: 20,
                  onPressed: () {
                    widget.webviewController.reload(true);
                  },
                ),
                Row(
                  children: [
                    IconButton(
                      icon: const Icon(Icons.zoom_in),
                      tooltip: 'Zoom in',
                      splashRadius: 20,
                      onPressed: () {
                        if (zoom < 1) {
                          var level = dp(zoom + 0.1, 1);
                          widget.webviewController.setZoomLevel(level);
                          setState(() {
                            zoom = level;
                          });
                        }
                      },
                    ),
                    Text("${100 + zoom * 100} %"),
                    IconButton(
                      icon: const Icon(Icons.zoom_out),
                      tooltip: 'Zoom out',
                      splashRadius: 20,
                      onPressed: () {
                        if (zoom > -1) {
                          var level = dp(zoom - 0.1, 1);
                          widget.webviewController.setZoomLevel(level);
                          setState(() {
                            zoom = level;
                          });
                        }
                      },
                    ),
                  ],
                ),
                IconButton(
                  icon: const Icon(Icons.code),
                  tooltip: 'Execute alert',
                  splashRadius: 20,
                  onPressed: () {
                    var message = json.encode({
                      "type": "event",
                      "timestamp": DateTime.now().toIso8601String()
                    });
                    widget.webviewController
                        .executeJavaScript("window.postMessage('$message')");
                  },
                ),
                IconButton(
                  icon: const Icon(Icons.developer_mode),
                  tooltip: 'Open DevTools',
                  splashRadius: 20,
                  onPressed: () {
                    // _controller.openDevTools();
                  },
                )
              ]),
            ),
            Center(
                child: StreamBuilder<String>(
              stream: widget.webviewController.title,
              builder: (context, snapshot) {
                return Text(
                    "Title: ${snapshot.hasData ? snapshot.data! : 'No title'}");
              },
            )),
            Expanded(
                child: Card(
                    color: Colors.transparent,
                    elevation: 0,
                    clipBehavior: Clip.antiAliasWithSaveLayer,
                    child: Stack(
                      children: [
                        Webview(
                          widget.webviewController,
                        ),
                        StreamBuilder<LoadingState>(
                            stream: widget.webviewController.loadingState,
                            builder: (context, snapshot) {
                              if (snapshot.hasData &&
                                  snapshot.data == LoadingState.inProcess) {
                                return const LinearProgressIndicator();
                              } else {
                                return const SizedBox();
                              }
                            }),
                      ],
                    ))),
          ],
        ),
      );
    }
  }

  @override
  bool get wantKeepAlive => true;
}
