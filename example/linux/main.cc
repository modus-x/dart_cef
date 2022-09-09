#include "my_application.h"
#include "dart_cef/dart_cef_plugin.h"
#include <iostream>



int main(int argc, char *argv[])
{
  char **argv_copy = copyArgv(argc, argv);

  int result_code = initCef(argc, argv);
  if (result_code != -1)
  {
    return result_code;
  }

  g_autoptr(MyApplication) app = my_application_new();
  GTK_WIDGET(app);

  g_timeout_add(200, runTasks, NULL);
  g_idle_add(runTasks, NULL);

  return g_application_run(G_APPLICATION(app), argc, argv_copy);
}


