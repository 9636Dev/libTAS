/*
    Copyright 2015-2020 Clément Gallet <clement.gallet@ens-lyon.org>

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
#include "../logging.h"
#include "inputs.h"
#include "../XcbEventQueueList.h"
#include "../DeterministicTimer.h"
#include <cstring> // memset
#include "../../shared/AllInputs.h"

namespace libtas {

/* Override */ xcb_query_pointer_cookie_t xcb_query_pointer (xcb_connection_t *c, xcb_window_t window)
{
    DEBUGLOGCALL(LCF_MOUSE);
    xcb_query_pointer_cookie_t pointer_cookie = {0};
    return pointer_cookie;
}

/* Override */ xcb_query_pointer_cookie_t xcb_query_pointer_unchecked (xcb_connection_t *c, xcb_window_t window)
{
    DEBUGLOGCALL(LCF_MOUSE);
    xcb_query_pointer_cookie_t pointer_cookie = {0};
    return pointer_cookie;
}

/* Override */ xcb_query_pointer_reply_t *xcb_query_pointer_reply (xcb_connection_t *c, xcb_query_pointer_cookie_t cookie, xcb_generic_error_t **e)
{
    DEBUGLOGCALL(LCF_MOUSE);
    xcb_query_pointer_reply_t* reply = new xcb_query_pointer_reply_t;
    reply->response_type = XCB_QUERY_POINTER;
    reply->sequence = 0;
    reply->same_screen = true;
    // reply->root = ; TODO
    reply->root_x = game_ai.pointer_x;
    reply->root_y = game_ai.pointer_y;
    reply->child = gameXWindows.front();
    reply->win_x = game_ai.pointer_x;
    reply->win_y = game_ai.pointer_y;
    reply->mask = SingleInput::toXlibPointerMask(game_ai.pointer_mask);
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
    debuglog(LCF_MOUSE, __func__, " called with dest_w ", dst_window, " and dest_x ", dst_x, " and dest_y ", dst_y);

    /* Does this generate a XCB_MOTION_NOTIFY event? */
    if (!gameXWindows.empty()) {
        xcb_motion_notify_event_t event;
        event.response_type = XCB_MOTION_NOTIFY;
        event.state = SingleInput::toXlibPointerMask(game_ai.pointer_mask);

        if (dst_window == XCB_NONE) {
            /* Relative warp */
            event.event_x = game_ai.pointer_x + dst_x;
            event.event_y = game_ai.pointer_y + dst_y;
        }
        else {
            /* Absolute warp */
            event.event_x = dst_x;
            event.event_y = dst_y;
        }
        event.root_x = event.event_x;
        event.root_y = event.event_y;
        event.event = gameXWindows.front();

        struct timespec time = detTimer.getTicks();
        event.time = time.tv_sec * 1000 + time.tv_nsec / 1000000;

        xcbEventQueueList.insert(reinterpret_cast<xcb_generic_event_t*>(&event));
        debuglog(LCF_EVENTS | LCF_MOUSE, "Generate xcb event XCB_MOTION_NOTIFY with new position (", game_ai.pointer_x, ",", game_ai.pointer_y,")");
    }

    /* Update the pointer coordinates */
    if (dst_window == XCB_NONE) {
        /* Relative warp */
        game_ai.pointer_x += dst_x;
        game_ai.pointer_y += dst_y;
    }
    else {
        /* Absolute warp */
        game_ai.pointer_x = dst_x;
        game_ai.pointer_y = dst_y;
    }

    xcb_void_cookie_t cookie{0};
    return cookie;
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
    debuglog(LCF_MOUSE, __func__, " called with dest_w ", dst_window, " and dest_x ", dst_x, " and dest_y ", dst_y);

    /* Does this generate a XCB_MOTION_NOTIFY event? */
    if (!gameXWindows.empty()) {
        xcb_motion_notify_event_t event;
        event.response_type = XCB_MOTION_NOTIFY;
        event.state = SingleInput::toXlibPointerMask(game_ai.pointer_mask);

        if (dst_window == XCB_NONE) {
            /* Relative warp */
            event.event_x = game_ai.pointer_x + dst_x;
            event.event_y = game_ai.pointer_y + dst_y;
        }
        else {
            /* Absolute warp */
            event.event_x = dst_x;
            event.event_y = dst_y;
        }
        event.root_x = event.event_x;
        event.root_y = event.event_y;
        event.event = gameXWindows.front();

        struct timespec time = detTimer.getTicks();
        event.time = time.tv_sec * 1000 + time.tv_nsec / 1000000;

        xcbEventQueueList.insert(reinterpret_cast<xcb_generic_event_t*>(&event));
        debuglog(LCF_EVENTS | LCF_MOUSE, "Generate xcb event XCB_MOTION_NOTIFY with new position (", game_ai.pointer_x, ",", game_ai.pointer_y,")");
    }

    /* Update the pointer coordinates */
    if (dst_window == XCB_NONE) {
        /* Relative warp */
        game_ai.pointer_x += dst_x;
        game_ai.pointer_y += dst_y;
    }
    else {
        /* Absolute warp */
        game_ai.pointer_x = dst_x;
        game_ai.pointer_y = dst_y;
    }

    xcb_void_cookie_t cookie{0};
    return cookie;
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
    DEBUGLOGCALL(LCF_MOUSE);
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
    DEBUGLOGCALL(LCF_MOUSE);
    xcb_grab_pointer_cookie_t pointer_grab_cookie = {0};
    return pointer_grab_cookie;
}

/* Override */ xcb_grab_pointer_reply_t *
xcb_grab_pointer_reply (xcb_connection_t           *c,
                        xcb_grab_pointer_cookie_t   cookie  /**< */,
                        xcb_generic_error_t       **e)
{
    DEBUGLOGCALL(LCF_MOUSE);
    xcb_grab_pointer_reply_t *reply = new xcb_grab_pointer_reply_t;
    reply->response_type = XCB_GRAB_POINTER;
    reply->sequence = 0;
    reply->status = XCB_GRAB_STATUS_SUCCESS;

    return reply;
}

/* Override */ xcb_void_cookie_t
xcb_ungrab_pointer_checked (xcb_connection_t *c,
                            xcb_timestamp_t   time)
{
    DEBUGLOGCALL(LCF_MOUSE);
    xcb_void_cookie_t mouse_ungrab_cookie = {0};
    return mouse_ungrab_cookie;
}

/* Override */ xcb_void_cookie_t
xcb_ungrab_pointer (xcb_connection_t *c,
                    xcb_timestamp_t   time)
{
    DEBUGLOGCALL(LCF_MOUSE);
    xcb_void_cookie_t mouse_ungrab_cookie = {0};
    return mouse_ungrab_cookie;
}

}
