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

#ifndef LIBTAS_XLIBEVENTQUEUE_H_INCLUDED
#define LIBTAS_XLIBEVENTQUEUE_H_INCLUDED

#include <list>
#include <map>
#include <mutex>
#include <X11/X.h>
#include <X11/Xlib.h>

namespace libtas {
/* This is a replacement of the Xlib event queue. */
class XlibEventQueue
{
    public:
        XlibEventQueue(Display* display);

        /* Set an event mask for a window */
        void setMask(Window w, long event_mask);

        /* Insert an event in the queue */
        int insert(XEvent* event);

        /* Copy the first event of the queue into `event`, and remove that
         * event from the queue if update is true. Returns if an event was pulled. */
        bool pop(XEvent* event, bool update);

        /* Copy the first event of the queue into `event` that matches the
         * window and event mask, and remove that event from the queue.
         * Returns if an event was pulled. */
        bool pop(XEvent* event, Window w, long event_mask);

        /* Copy the first event of the queue into `event` that matches the
         * window and event type, and remove that event from the queue.
         * Returns if an event was pulled. */
        bool pop(XEvent* event, Window w, int event_type);

        /* Copy the first event of the queue into `event` where `predicate`
         * evalues to true, and remove that event from the queue.
         * Returns if an event was pulled. */
        bool pop(XEvent* event, Bool (*predicate)(Display *, XEvent *, XPointer), XPointer arg);

        /* Return the size of the queue */
        int size();

        /* Mimick a pointer grab by redirecting pointer events to the grab window */
        void grabPointer(Window window, unsigned int event_mask, bool owner_events);

        /* Ungrab pointer  */
        void ungrabPointer();

        Display* display;

        /* Was the queue emptied? Used for asynchronous events */
        bool emptied;

        /* Mutex for protecting empied and pop() */
        std::recursive_mutex mutex;

    private:
        /* Event queue */
        std::list<XEvent> eventQueue;

        /* Event mask for each Window */
        std::map<Window, long> eventMasks;

        /* Does a type belong to an event mask?*/
        bool isTypeOfMask(int type, long event_mask);
        
        Window grab_window;
        unsigned int grab_event_mask;
        bool grab_owner_events;
};

}

#endif
