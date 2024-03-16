/*
    Copyright 2015-2023 Clément Gallet <clement.gallet@ens-lyon.org>

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

#include "inputs.h"

#include "global.h"
#include "logging.h"

namespace libtas {

AllInputsFlat ai;
AllInputsFlat old_ai;
AllInputsFlat game_ai;
AllInputsFlat old_game_ai;
AllInputsFlat game_unclipped_ai;
AllInputsFlat old_game_unclipped_ai;

bool pointer_clipping = false;
int clipping_x, clipping_y, clipping_w, clipping_h;

void updateGameInputs()
{
    old_game_ai = game_ai;
    old_game_unclipped_ai = game_unclipped_ai;

    game_ai.keyboard = ai.keyboard;

    game_ai.pointer.mode = ai.pointer.mode;
    if (game_ai.pointer.mode == SingleInput::POINTER_MODE_RELATIVE) {
        game_ai.pointer.x += ai.pointer.x;
        game_ai.pointer.y += ai.pointer.y;
        game_unclipped_ai.pointer.x += ai.pointer.x;
        game_unclipped_ai.pointer.y += ai.pointer.y;
    }
    else {
        /* If we just switch to absolute, keep the same coords for that frame */
        if (old_game_ai.pointer.mode == SingleInput::POINTER_MODE_RELATIVE) {
            game_ai.pointer.x = old_game_ai.pointer.x;
            game_ai.pointer.y = old_game_ai.pointer.y;
            game_unclipped_ai.pointer.x = game_unclipped_ai.pointer.x;
            game_unclipped_ai.pointer.y = game_unclipped_ai.pointer.y;
        }
        else {
            game_ai.pointer.x += ai.pointer.x - old_ai.pointer.x;
            game_ai.pointer.y += ai.pointer.y - old_ai.pointer.y;
            game_unclipped_ai.pointer.x += ai.pointer.x - old_ai.pointer.x;
            game_unclipped_ai.pointer.y += ai.pointer.y - old_ai.pointer.y;
        }
    }

    game_ai.pointer.wheel = ai.pointer.wheel;
    game_ai.pointer.mask = ai.pointer.mask;

    for (int ji=0; ji<Global::shared_config.nb_controllers; ji++) {
        game_ai.controllers[ji] = ai.controllers[ji];
    }

    /* Clipping pointer inside grab window */
    if (pointer_clipping) {
        if (game_ai.pointer.x < clipping_x)
            game_ai.pointer.x = clipping_x;
        else if (game_ai.pointer.x >= (clipping_x + clipping_w))
            game_ai.pointer.x = clipping_x + clipping_w - 1;

        if (game_ai.pointer.y < clipping_y)
            game_ai.pointer.y = clipping_y;
        else if (game_ai.pointer.y >= (clipping_y + clipping_h))
            game_ai.pointer.y = clipping_y + clipping_h - 1;
    }

    game_ai.misc.flags = ai.misc.flags;

    old_ai = ai;
}

}
