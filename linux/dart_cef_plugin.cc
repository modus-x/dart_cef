#include "include/dart_cef/dart_cef_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <sys/utsname.h>

#include <cstring>
#include <iostream>

#include <gtk/gtk.h>

#include <X11/Xlib.h>
#undef Success    // Definition conflicts with cef_message_router.h
#undef RootWindow // Definition conflicts with root_window.h

#include <stdlib.h>
#include <unistd.h>
#include <memory>

#include "include/base/cef_logging.h"
#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include "client_browser.h"
#include "main_message_loop_multithreaded_gtk.h"
#include "main_message_loop.h"
#include "client_app_other.h"
#include "client_switches.h"
#include "client_renderer.h"

#define DART_CEF_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), dart_cef_plugin_get_type(), \
                              DartCefPlugin))

struct _DartCefPlugin
{
  GObject parent_instance;
  FlMethodChannel *method_channel;
  FlTextureRegistrar *texture_registrar;
};

G_DEFINE_TYPE(DartCefPlugin, dart_cef_plugin, g_object_get_type())

// Called when a method call is received from Flutter.
static void dart_cef_plugin_handle_method_call(
    DartCefPlugin *self,
    FlMethodCall *method_call)
{
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar *method = fl_method_call_get_name(method_call);

  if (strcmp(method, "getPlatformVersion") == 0)
  {
    struct utsname uname_data = {};
    uname(&uname_data);
    g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
    g_autoptr(FlValue) result = fl_value_new_string(version);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  }
  else
  {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void dart_cef_plugin_dispose(GObject *object)
{
  G_OBJECT_CLASS(dart_cef_plugin_parent_class)->dispose(object);
}

static void dart_cef_plugin_class_init(DartCefPluginClass *klass)
{
  G_OBJECT_CLASS(klass)->dispose = dart_cef_plugin_dispose;
}

static void dart_cef_plugin_init(DartCefPlugin *self) {}

static void method_call_cb(FlMethodChannel *channel, FlMethodCall *method_call,
                           gpointer user_data)
{
  DartCefPlugin *plugin = DART_CEF_PLUGIN(user_data);
  dart_cef_plugin_handle_method_call(plugin, method_call);
}

void dart_cef_plugin_register_with_registrar(FlPluginRegistrar *registrar)
{
  DartCefPlugin *plugin = DART_CEF_PLUGIN(
      g_object_new(dart_cef_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();

  plugin->method_channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "dart_vlc", FL_METHOD_CODEC(codec));
  plugin->texture_registrar =
      fl_plugin_registrar_get_texture_registrar(registrar);
  fl_method_channel_set_method_call_handler(plugin->method_channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}
namespace client
{
  namespace
  {

    // int XErrorHandlerImpl(Display *display, XErrorEvent *event)
    // {
    //   LOG(WARNING) << "X error received: "
    //                << "type " << event->type << ", "
    //                << "serial " << event->serial << ", "
    //                << "error_code " << static_cast<int>(event->error_code) << ", "
    //                << "request_code " << static_cast<int>(event->request_code)
    //                << ", "
    //                << "minor_code " << static_cast<int>(event->minor_code);
    //   return 0;
    // }

    // int XIOErrorHandlerImpl(Display *display)
    // {
    //   return 0;
    // }

    // void TerminationSignalHandler(int signatl)
    // {
    //   LOG(ERROR) << "Received termination signal: " << signatl;
    // }

    int initCef(int argc, char *argv[])
    {
      CefMainArgs main_args(argc, argv);

      // Parse command-line arguments.
      CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();

      // Create a ClientApp of the correct type.
      CefRefPtr<CefApp> app;
      // std::cout << "browser process  " << std::endl;
      // ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
      // std::cout << "browser process  " << std::endl;
      // if (process_type == ClientApp::BrowserProcess)
      // {
      //   app = new ClientAppBrowser();
      // }
      // else if (process_type == ClientApp::RendererProcess ||
      //          process_type == ClientApp::ZygoteProcess)
      // {
      //   // On Linux the zygote process is used to spawn other process types. Since
      //   // we don't know what type of process it will be give it the renderer
      //   // client.
      //   app = new ClientAppRenderer();
      // }
      // else if (process_type == ClientApp::OtherProcess)
      // {
      //   app = new ClientAppOther();
      // }

      // Execute the secondary process, if any.
      int exit_code = CefExecuteProcess(main_args, app, nullptr);
      if (exit_code >= 0)
        return exit_code;

      CefSettings settings;

      // When generating projects with CMake the CEF_USE_SANDBOX value will be defined
      // automatically. Pass -DUSE_SANDBOX=OFF to the CMake command-line to disable
      // use of the sandbox.
      settings.no_sandbox = true;
      settings.windowless_rendering_enabled = true;
      settings.multi_threaded_message_loop = true;

      if (settings.windowless_rendering_enabled)
      {
        // Force the app to use OpenGL <= 3.1 when off-screen rendering is enabled.
        // TODO(cefclient): Rewrite OSRRenderer to use shaders instead of the
        // fixed-function pipeline which was removed in OpenGL 3.2 (back in 2009).
        setenv("MESA_GL_VERSION_override", "3.1", /*overwrite=*/0);
      }

      // Initialize CEF.
      CefInitialize(main_args, settings, app, nullptr);

      return exit_code;
    }

  }
}

 int initCef(int argc, char *argv[]) {
  return client::initCef(argc, argv);
 }