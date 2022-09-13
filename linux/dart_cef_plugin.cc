#include "include/dart_cef/dart_cef_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <sys/utsname.h>

#include <cstring>
#include <iostream>
#include <thread>

#include <gtk/gtk.h>

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
#include "data.h"
#include "simple_handler.h"

#define DART_CEF_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), dart_cef_plugin_get_type(), \
                              DartCefPlugin))

CefRefPtr<SimpleHandler> handler(new SimpleHandler());

struct _DartCefPlugin
{
  GObject parent_instance;
  FlMethodChannel *method_channel;
  FlTextureRegistrar *texture_registrar;
  FlBinaryMessenger *messenger;
};

G_DEFINE_TYPE(DartCefPlugin, dart_cef_plugin, g_object_get_type())

namespace client
{

  GtkWidget *parent = nullptr;

  namespace
  {

    MainMessageLoopMultithreadedGtk loop;

    int initCef(int argc, char *argv[])
    {
      CefMainArgs main_args(argc, argv);

      // Parse command-line arguments.
      CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
      command_line->InitFromArgv(argc, argv);

      // Create a ClientApp of the correct type.
      CefRefPtr<CefApp> app;
      ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
      if (process_type == ClientApp::BrowserProcess)
      {
        std::cout << "browser process!" << std::endl;
        app = new ClientAppBrowser();
      }
      else if (process_type == ClientApp::RendererProcess ||
               process_type == ClientApp::ZygoteProcess)
      {
        // On Linux the zygote process is used to spawn other process types. Since
        // we don't know what type of process it will be give it the renderer
        // client.
        app = new ClientAppRenderer();
      }
      else if (process_type == ClientApp::OtherProcess)
      {
        app = new ClientAppOther();
      }

      // Execute the secondary process, if any.
      int exit_code = CefExecuteProcess(main_args, app, nullptr);
      if (exit_code >= 0)
        return exit_code;

      CefSettings settings;

      settings.no_sandbox = true;
      settings.windowless_rendering_enabled = true;
      settings.multi_threaded_message_loop = true;
      settings.remote_debugging_port = 8088;
      CefString(&settings.log_file).FromASCII("webview_cef.log");

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

    void runTasks()
    {
      loop.RunTasks();
    }

    void setParent(GtkWidget *widget)
    {
      parent = widget;
    }

    GtkWidget *getParent()
    {
      return parent;
    }
  }
}

bool sendKeyEvent(GdkEventKey *event) {
  return SimpleHandler::GetInstance()->sendKeyEvent(event);
}

int initCef(int argc, char *argv[])
{
  return client::initCef(argc, argv);
}

char **copyArgv(int argc, char *argv[])
{
  CefScopedArgArray scoped_arg_array(argc, argv);
  char **argv_copy = scoped_arg_array.array();
  return argv_copy;
}

int runTasks(void *self)
{
  client::runTasks();
  return G_SOURCE_CONTINUE;
}

void setParentWindow(GtkWidget *parent)
{
  client::setParent(parent);
}

// Called when a method call is received from Flutter.
static void dart_cef_plugin_handle_method_call(
    DartCefPlugin *self,
    FlMethodCall *method_call)
{
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar *method = fl_method_call_get_name(method_call);
  FlValue *args = fl_method_call_get_args(method_call);

  if (strcmp(method, "getPlatformVersion") == 0)
  {
    struct utsname uname_data = {};
    uname(&uname_data);
    g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
    g_autoptr(FlValue) result = fl_value_new_string(version);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  }
  else if (strcmp(method, "createBrowser") == 0)
  {
    std::string url = fl_value_get_string(
        fl_value_lookup_string(args, "startUrl"));
    std::string bind_func = fl_value_get_string(
        fl_value_lookup_string(args, "webMessageFunction"));
    std::string token = fl_value_get_string(
        fl_value_lookup_string(args, "token"));
    std::string access_token = fl_value_get_string(
        fl_value_lookup_string(args, "accessToken"));
    bool is_string = fl_value_get_bool(
        fl_value_lookup_string(args, "isHTML"));
    if (is_string)
    {
      url = GetDataURI(url, "text/html");
    }

    int64_t texture_id = handler->createBrowser(self->messenger, self->texture_registrar, url, bind_func, token, access_token, client::getParent());
    LOG(INFO) << "Create browser request for " << texture_id << " texture and url " << url;
    g_autoptr(FlValue) result = fl_value_new_int(texture_id);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  }

  else if (strcmp(method, "closeAllBrowsers") == 0)
  {
    bool force = fl_value_get_bool(args);
    handler->CloseAllBrowsers(force);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(NULL));
  }
  else if (strcmp(method, "shutdown") == 0)
  {
    CefShutdown();
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(NULL));
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

  plugin->messenger = fl_plugin_registrar_get_messenger(registrar);

  plugin->method_channel =
      fl_method_channel_new(plugin->messenger,
                            "webview_cef", FL_METHOD_CODEC(codec));

  plugin->texture_registrar =
      fl_plugin_registrar_get_texture_registrar(registrar);
  fl_method_channel_set_method_call_handler(plugin->method_channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);

  LOG(INFO) << "Successfully loaded webview_cef plugin" << std::endl;
}
