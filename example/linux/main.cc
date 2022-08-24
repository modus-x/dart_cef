#include "my_application.h"
#include "dart_cef/dart_cef_plugin.h"

int main(int argc, char* argv[])
{
  int result_code = initCef(argc, argv);
  if (result_code != -1)
  {
    return result_code;
  }
  g_autoptr(MyApplication) app = my_application_new();
  return g_application_run(G_APPLICATION(app), argc, argv);
}
