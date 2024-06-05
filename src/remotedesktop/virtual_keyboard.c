#include <xkbcommon/xkbcommon.h>

#include "virtual_keyboard.h"

#include "virtual-keyboard-unstable-v1-client-protocol.h"

#include "remotedesktop.h"
#include "xdpw.h"
#include "logger.h"

static void wlr_registry_handle_add(void *data, struct wl_registry *reg,
		uint32_t id, const char *interface, uint32_t ver) {
	struct xdpw_remotedesktop_context *ctx = data;

	logprint(DEBUG, "wlroots: interface to register %s  (Version: %u)",
		interface, ver);

	if (!strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name)) {
		uint32_t version = ver;
		if (VIRTUAL_KEYBOARD_VERSION < ver) {
			version = VIRTUAL_KEYBOARD_VERSION;
		} else if (ver < VIRTUAL_KEYBOARD_VERSION_MIN) {
			version = VIRTUAL_KEYBOARD_VERSION_MIN;
		}
		logprint(DEBUG,
			"wlroots: |-- registered to interface %s (Version %u)",
			interface, version);
		ctx->virtual_keyboard_manager = wl_registry_bind(reg, id,
			&zwp_virtual_keyboard_manager_v1_interface, version);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		ctx->seat = wl_registry_bind(reg, id, &wl_seat_interface, 1);
	}
}

static const struct wl_registry_listener wlr_registry_listener = {
	.global = wlr_registry_handle_add,
	.global_remove = NULL,
};

struct xkb_context *xkb_ctx;
struct xkb_keymap *xkb_keymap;
struct xkb_state* xkb_state;

int xdpw_virtual_keyboard_init(struct xdpw_state *state) {
	struct xdpw_remotedesktop_context *ctx = &state->remotedesktop;

	// retrieve registry
	ctx->registry = wl_display_get_registry(state->wl_display);
	wl_registry_add_listener(ctx->registry, &wlr_registry_listener, ctx);

	wl_display_roundtrip(state->wl_display);

	logprint(DEBUG, "wayland: registry listeners run");
	wl_display_roundtrip(state->wl_display);

	// make sure our wlroots supports virtual-keyboard protocol
	if (!ctx->virtual_keyboard_manager) {
		logprint(ERROR, "Compositor doesn't support %s!",
			zwp_virtual_keyboard_manager_v1_interface.name);
		return -1;
	}

	struct xkb_rule_names names = {
        .rules = "",
        .model = "",
        .layout = "us",
        .variant = "",
        .options = ""
    };

	xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	xkb_keymap = xkb_keymap_new_from_names(
		xkb_ctx,
		&names,
		XKB_KEYMAP_COMPILE_NO_FLAGS
	);

	xkb_state = xkb_state_new(xkb_keymap);

	return 0;
}

void xdpw_virtual_keyboard_finish(struct xdpw_remotedesktop_context *ctx) {
	if (ctx->virtual_keyboard_manager) {
		zwp_virtual_keyboard_manager_v1_destroy(ctx->virtual_keyboard_manager);
	}
}
