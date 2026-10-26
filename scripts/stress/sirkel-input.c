// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// Sends fake keyboard and pointer input to a KWin through the
// org_kde_kwin_fake_input protocol, for the stress test. Reads one command
// per line from stdin:
//   key <evdev code> <1|0>    press or release a key (F24 = 194)
//   move <dx> <dy>            relative pointer motion
//   abs <x> <y>               absolute pointer position
//   button <code> <1|0>       press or release a mouse button (left = 272)
//   sleep <ms>
//   sync                      wait until KWin has handled everything
// KWin only offers the protocol to trusted clients; the stress test runs its
// KWin with KWIN_WAYLAND_NO_PERMISSION_CHECKS=1.

#include "fake-input-client-protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-client.h>

static struct org_kde_kwin_fake_input *fake;

static void global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    (void)data;
    if (strcmp(interface, org_kde_kwin_fake_input_interface.name) == 0) {
        fake = wl_registry_bind(registry, name, &org_kde_kwin_fake_input_interface, version < 4 ? version : 4);
    }
}

static void global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {global, global_remove};

static void sleep_ms(long ms)
{
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

int main(void)
{
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "sirkel-input: can't connect to the Wayland display\n");
        return 1;
    }
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    if (!fake) {
        fprintf(stderr, "sirkel-input: org_kde_kwin_fake_input isn't offered (permission checks on?)\n");
        return 1;
    }
    org_kde_kwin_fake_input_authenticate(fake, "sirkel-stress", "Sirkel stress test");
    wl_display_roundtrip(display);

    char line[256];
    while (fgets(line, sizeof line, stdin)) {
        char cmd[16];
        double a = 0, b = 0;
        int n = sscanf(line, "%15s %lf %lf", cmd, &a, &b);
        if (n < 1) {
            continue;
        }
        if (strcmp(cmd, "key") == 0 && n == 3) {
            org_kde_kwin_fake_input_keyboard_key(fake, (uint32_t)a, (uint32_t)b);
        } else if (strcmp(cmd, "move") == 0 && n == 3) {
            org_kde_kwin_fake_input_pointer_motion(fake, wl_fixed_from_double(a), wl_fixed_from_double(b));
        } else if (strcmp(cmd, "button") == 0 && n == 3) {
            org_kde_kwin_fake_input_button(fake, (uint32_t)a, (uint32_t)b);
        } else if (strcmp(cmd, "abs") == 0 && n == 3) {
            org_kde_kwin_fake_input_pointer_motion_absolute(fake, wl_fixed_from_double(a), wl_fixed_from_double(b));
        } else if (strcmp(cmd, "sleep") == 0 && n >= 2) {
            wl_display_flush(display);
            sleep_ms((long)a);
            continue;
        } else if (strcmp(cmd, "sync") == 0) {
            wl_display_roundtrip(display);
            fprintf(stdout, "synced\n");
            fflush(stdout);
            continue;
        }
        if (wl_display_flush(display) < 0) {
            fprintf(stderr, "sirkel-input: lost the connection to KWin\n");
            return 2;
        }
    }
    wl_display_roundtrip(display);
    wl_display_disconnect(display);
    return 0;
}
