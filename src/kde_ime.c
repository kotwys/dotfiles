/*
  This small program is aimed to synchronize the state of Fcitx
  and the current KDE keyboard layout.  When the layout is switched,
  the corresponding Fcitx input method group is activated.

  For this program to work properly, Fcitx's layout override option
  should be disabled.
*/

#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

static const char* get_group_for_layout(uint32_t layout_id)
{
  if (layout_id == 0) return "Default"; // English
  return "Disabled";
}

static int on_layout_changed(sd_bus_message *m,
                             void *userdata,
                             sd_bus_error *ret_error)
{
  sd_bus *bus = sd_bus_message_get_bus(m);
  uint32_t layout_id;
  int r;

  r = sd_bus_message_read(m, "u", &layout_id);
  if (r < 0)
    return r;

  const char *group = get_group_for_layout(layout_id);
  r = sd_bus_call_method(bus,
                         "org.fcitx.Fcitx5",
                         "/controller",
                         "org.fcitx.Fcitx.Controller1",
                         "SwitchInputMethodGroup",
                         NULL, NULL, "s", group);
  if (r < 0) {
    fprintf(stderr, "Failed to switch Fcitx5 group: %s\n", strerror(-r));
    return r;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  sd_bus *bus = NULL;
  int r;

  r = sd_bus_default_user(&bus);
  if (r < 0) {
    fprintf(stderr, "Failed to connect to session bus: %s\n", strerror(-r));
    return EXIT_FAILURE;
  }

  r = sd_bus_match_signal(bus, NULL,
                          "org.kde.keyboard",
                          "/Layouts",
                          "org.kde.KeyboardLayouts",
                          "layoutChanged",
                          on_layout_changed, NULL);
  if (r < 0) {
    fprintf(stderr, "Failed to add match rule: %s\n", strerror(-r));
    goto finish;
  }

  r = sd_bus_call_method_async(bus, NULL,
                               "org.kde.keyboard",
                               "/Layouts",
                               "org.kde.KeyboardLayouts",
                               "getLayout",
                               on_layout_changed, NULL, "");
  if (r < 0) {
    fprintf(stderr, "Could not fetch current layout: %s\n", strerror(-r));
    goto finish;
  }

  printf("Monitoring KDE Keyboard Layout... Press Ctrl+C to stop.\n");

  for (;;) {
    r = sd_bus_process(bus, NULL);
    if (r < 0) {
      fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
      goto finish;
    }
    if (r > 0)
      continue;

    r = sd_bus_wait(bus, UINT64_MAX);
    if (r < 0) {
      fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
      goto finish;
    }
  }

 finish:
  sd_bus_unref(bus);
  return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
