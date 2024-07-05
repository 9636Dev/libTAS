/*
    Copyright 2015-2024 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xcbpointer.h"
#include "inputs.h"

#include "logging.h"
#include "xcb/XcbEventQueueList.h"
#include "DeterministicTimer.h"
#include "hook.h"
#include "xlib/xwindows.h" // x11::gameXWindows
#include "global.h"
#include "GlobalState.h"

#include <cstring> // memset

namespace libtas {

/* Override */ xcb_query_pointer_cookie_t xcb_query_pointer (xcb_connection_t *c, xcb_window_t window)
{
    LOGTRACE(LCF_MOUSE);
    xcb_query_pointer_cookie_t pointer_cookie = {0};
    return pointer_cookie;
}

/* Override */ xcb_query_pointer_cookie_t xcb_query_pointer_unchecked (xcb_connection_t *c, xcb_window_t window)
{
    LOGTRACE(LCF_MOUSE);
    xcb_query_pointer_cookie_t pointer_cookie = {0};
    return pointer_cookie;
}

/* Override */ xcb_query_pointer_reply_t *xcb_query_pointer_reply (xcb_connection_t *c, xcb_query_pointer_cookie_t cookie, xcb_generic_error_t **e)
{
    LOGTRACE(LCF_MOUSE);
    auto* reply = static_cast<xcb_query_pointer_reply_t*>(malloc(sizeof(xcb_query_pointer_reply_t)));
    reply->response_type = XCB_QUERY_POINTER;
    reply->sequence = 0;
    reply->same_screen = true;
    // reply->root = ; TODO
    reply->root_x = Inputs::game_ai.pointer.x;
    reply->root_y = Inputs::game_ai.pointer.y;
    reply->child = x11::gameXWindows.front();
    reply->win_x = Inputs::game_ai.pointer.x;
    reply->win_y = Inputs::game_ai.pointer.y;
    reply->mask = SingleInput::toXlibPointerMask(Inputs::game_ai.pointer.mask);
    return reply;
}

/* Override */ xcb_void_cookie_t
xcb_warp_pointer_checked (xcb_connection_t *c,
                          xcb_window_t      src_window,
                          xcb_window_t      dst_window,
                          int16_t           src_x,
                          int16_t           src_y,
                          uint16_t          src_width,
                          uint16_t          src_height,
                          int16_t           dst_x,
                          int16_t           dst_y)
{
    RETURN_IF_NATIVE(xcb_warp_pointer_checked, (c, src_window, dst_window, src_x, src_y, src_width, src_height, dst_x, dst_y), nullptr);

    LOG(LL_TRACE, LCF_MOUSE, "%s called with dest_w %d and dest_x %d and dest_y %d", __func__, dst_window, dst_x, dst_y);

    /* Does this generate a XCB_MOTION_NOTIFY event? */
    if (!x11::gameXWindows.empty()) {
        xcb_motion_notify_event_t event;
        event.response_type = XCB_MOTION_NOTIFY;
        event.state = SingleInput::toXlibPointerMask(Inputs::game_ai.pointer.mask);

        if (dst_window == XCB_NONE) {
            /* Relative warp */
            event.event_x = Inputs::game_ai.pointer.x + dst_x;
            event.event_y = Inputs::game_ai.pointer.y + dst_y;
        }
        else {
            /* Absolute warp */
            event.event_x = dst_x;
            event.event_y = dst_y;
        }
        event.root_x = event.event_x;
        event.root_y = event.event_y;
        event.event = x11::gameXWindows.front();

        struct timespec time = DeterministicTimer::get().getTicks();
        event.time = time.tv_sec * 1000 + time.tv_nsec / 1000000;

        xcbEventQueueList.insert(reinterpret_cast<xcb_generic_event_t*>(&event), false);
        LOG(LL_DEBUG, LCF_EVENTS | LCF_MOUSE, "Generate xcb event XCB_MOTION_NOTIFY with new position (%d,%d)", Inputs::game_ai.pointer.x, Inputs::game_ai.pointer.y);
    }

    /* Update the pointer coordinates */
    if (dst_window == XCB_NONE) {
        /* Relative warp */
        Inputs::game_ai.pointer.x += dst_x;
        Inputs::game_ai.pointer.y += dst_y;
    }
    else {
        /* Absolute warp */
        Inputs::game_ai.pointer.x = dst_x;
        Inputs::game_ai.pointer.y = dst_y;
    }

    if (Global::shared_config.mouse_prevent_warp) {
        xcb_void_cookie_t cookie{0};
        return cookie;        
    }
    
    /* When warping cursor, real and game cursor position are now synced */
    if (Global::shared_config.mouse_support) {
        if (dst_window == XCB_NONE) {
            /* Relative warp */
            Inputs::old_ai.pointer.x += dst_x;
            Inputs::old_ai.pointer.y += dst_y;
        }
        else {
            /* Absolute warp */
            Inputs::old_ai.pointer.x = dst_x;
            Inputs::old_ai.pointer.y = dst_y;
        }
    }

    RETURN_NATIVE(xcb_warp_pointer_checked, (c, src_window, dst_window, src_x, src_y, src_width, src_height, dst_x, dst_y), nullptr);
}

/* Override */ xcb_void_cookie_t
xcb_warp_pointer (xcb_connection_t *c,
                  xcb_window_t      src_window,
                  xcb_window_t      dst_window,
                  int16_t           src_x,
                  int16_t           src_y,
                  uint16_t          src_width,
                  uint16_t          src_height,
                  int16_t           dst_x,
                  int16_t           dst_y)
{
    RETURN_IF_NATIVE(xcb_warp_pointer, (c, src_window, dst_window, src_x, src_y, src_width, src_height, dst_x, dst_y), nullptr);
    
    LOG(LL_TRACE, LCF_MOUSE, "%s called with dest_w %d and dest_x %d and dest_y %d", __func__, dst_window, dst_x, dst_y);

    /* Does this generate a XCB_MOTION_NOTIFY event? */
    if (!x11::gameXWindows.empty()) {
        xcb_motion_notify_event_t event;
        event.response_type = XCB_MOTION_NOTIFY;
        event.state = SingleInput::toXlibPointerMask(Inputs::game_ai.pointer.mask);

        if (dst_window == XCB_NONE) {
            /* Relative warp */
            event.event_x = Inputs::game_ai.pointer.x + dst_x;
            event.event_y = Inputs::game_ai.pointer.y + dst_y;
        }
        else {
            /* Absolute warp */
            event.event_x = dst_x;
            event.event_y = dst_y;
        }
        event.root_x = event.event_x;
        event.root_y = event.event_y;
        event.event = x11::gameXWindows.front();

        struct timespec time = DeterministicTimer::get().getTicks();
        event.time = time.tv_sec * 1000 + time.tv_nsec / 1000000;

        xcbEventQueueList.insert(reinterpret_cast<xcb_generic_event_t*>(&event), false);
        LOG(LL_DEBUG, LCF_EVENTS | LCF_MOUSE, "Generate xcb event XCB_MOTION_NOTIFY with new position (%d,%d)", Inputs::game_ai.pointer.x, Inputs::game_ai.pointer.y);
    }

    /* Update the pointer coordinates */
    if (dst_window == XCB_NONE) {
        /* Relative warp */
        Inputs::game_ai.pointer.x += dst_x;
        Inputs::game_ai.pointer.y += dst_y;
    }
    else {
        /* Absolute warp */
        Inputs::game_ai.pointer.x = dst_x;
        Inputs::game_ai.pointer.y = dst_y;
    }

    if (Global::shared_config.mouse_prevent_warp) {
        xcb_void_cookie_t cookie{0};
        return cookie;        
    }
    
    /* When warping cursor, real and game cursor position are now synced */
    if (Global::shared_config.mouse_support) {
        if (dst_window == XCB_NONE) {
            /* Relative warp */
            Inputs::old_ai.pointer.x += dst_x;
            Inputs::old_ai.pointer.y += dst_y;
        }
        else {
            /* Absolute warp */
            Inputs::old_ai.pointer.x = dst_x;
            Inputs::old_ai.pointer.y = dst_y;
        }
    }

    RETURN_NATIVE(xcb_warp_pointer, (c, src_window, dst_window, src_x, src_y, src_width, src_height, dst_x, dst_y), nullptr);
}

/* Override */ xcb_grab_pointer_cookie_t
xcb_grab_pointer (xcb_connection_t *c,
                  uint8_t           owner_events,
                  xcb_window_t      grab_window,
                  uint16_t          event_mask,
                  uint8_t           pointer_mode,
                  uint8_t           keyboard_mode,
                  xcb_window_t      confine_to,
                  xcb_cursor_t      cursor,
                  xcb_timestamp_t   time)
{
    LOGTRACE(LCF_MOUSE);
    xcb_grab_pointer_cookie_t pointer_grab_cookie = {0};
    return pointer_grab_cookie;
}

/* Override */ xcb_grab_pointer_cookie_t
xcb_grab_pointer_unchecked (xcb_connection_t *c,
                            uint8_t           owner_events,
                            xcb_window_t      grab_window,
                            uint16_t          event_mask,
                            uint8_t           pointer_mode,
                            uint8_t           keyboard_mode,
                            xcb_window_t      confine_to,
                            xcb_cursor_t      cursor,
                            xcb_timestamp_t   time)
{
    LOGTRACE(LCF_MOUSE);
    xcb_grab_pointer_cookie_t pointer_grab_cookie = {0};
    return pointer_grab_cookie;
}

/* Override */ xcb_grab_pointer_reply_t *
xcb_grab_pointer_reply (xcb_connection_t           *c,
                        xcb_grab_pointer_cookie_t   cookie  /**< */,
                        xcb_generic_error_t       **e)
{
    LOGTRACE(LCF_MOUSE);
    auto* reply = static_cast<xcb_grab_pointer_reply_t*>(malloc(sizeof(xcb_grab_pointer_reply_t)));
    reply->response_type = XCB_GRAB_POINTER;
    reply->sequence = 0;
    reply->status = XCB_GRAB_STATUS_SUCCESS;

    return reply;
}

/* Override */ xcb_void_cookie_t
xcb_ungrab_pointer_checked (xcb_connection_t *c,
                            xcb_timestamp_t   time)
{
    LOGTRACE(LCF_MOUSE);
    xcb_void_cookie_t mouse_ungrab_cookie = {0};
    return mouse_ungrab_cookie;
}

/* Override */ xcb_void_cookie_t
xcb_ungrab_pointer (xcb_connection_t *c,
                    xcb_timestamp_t   time)
{
    LOGTRACE(LCF_MOUSE);
    xcb_void_cookie_t mouse_ungrab_cookie = {0};
    return mouse_ungrab_cookie;
}

}
