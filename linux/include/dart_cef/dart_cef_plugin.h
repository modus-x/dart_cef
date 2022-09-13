#ifndef FLUTTER_PLUGIN_DART_CEF_PLUGIN_H_
#define FLUTTER_PLUGIN_DART_CEF_PLUGIN_H_

#include <flutter_linux/flutter_linux.h>

G_BEGIN_DECLS

#ifdef FLUTTER_PLUGIN_IMPL
#define FLUTTER_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#define FLUTTER_PLUGIN_EXPORT
#endif

typedef struct _DartCefPlugin DartCefPlugin;
typedef struct
{
  GObjectClass parent_class;
} DartCefPluginClass;

FLUTTER_PLUGIN_EXPORT GType dart_cef_plugin_get_type();

FLUTTER_PLUGIN_EXPORT void dart_cef_plugin_register_with_registrar(
    FlPluginRegistrar *registrar);

FLUTTER_PLUGIN_EXPORT int initCef(int argc, char *argv[]);

FLUTTER_PLUGIN_EXPORT char **copyArgv(int argc, char *argv[]);

FLUTTER_PLUGIN_EXPORT int runTasks(void *self);

FLUTTER_PLUGIN_EXPORT void setParentWindow(GtkWidget* parent);

FLUTTER_PLUGIN_EXPORT bool sendKeyEvent(GdkEventKey *event);

G_END_DECLS

#endif // FLUTTER_PLUGIN_DART_CEF_PLUGIN_H_
