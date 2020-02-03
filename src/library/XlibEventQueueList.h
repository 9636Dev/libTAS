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

#ifndef LIBTAS_XLIBEVENTQUEUELIST_H_INCLUDED
#define LIBTAS_XLIBEVENTQUEUELIST_H_INCLUDED

#include <list>
#include <memory>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "XlibEventQueue.h"

namespace libtas {
/* This class stores all Xlib event queues. */
class XlibEventQueueList
{
    public:
        /* Create a new queue associated with a display */
        std::shared_ptr<XlibEventQueue> newQueue(Display* display);

        /* Delete a new queue associated with a display */
        void deleteQueue(Display* display);

        /* Return the queue associated with a display */
        std::shared_ptr<XlibEventQueue> getQueue(Display* display);

        /* Insert an event into a specific queue */
        int insert(Display* display, XEvent* event);

        /* Insert an event into all queues */
        void insert(XEvent* event);

        /* Wait for each queue to become empty */
        bool waitForEmpty();

        /* Reset the empty state of each queue */
        void resetEmpty();

        /* Lock and unlock the queue */
        void lock();
        void unlock();

    private:
        std::list<std::shared_ptr<XlibEventQueue>> eventQueueList;

};

extern XlibEventQueueList xlibEventQueueList;

}

#endif
