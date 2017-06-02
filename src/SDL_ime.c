/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "SDL_ime.h"
#include "SDL_fcitx.h"

typedef bool (*_SDL_IME_Init)();
typedef void (*_SDL_IME_Quit)();
typedef void (*_SDL_IME_SetFocus)(bool);
typedef void (*_SDL_IME_Reset)();
typedef bool (*_SDL_IME_ProcessKeyEvent)(uint32_t, Uint32);
typedef void (*_SDL_IME_UpdateTextRect)(SDL_Rect *);
typedef void (*_SDL_IME_PumpEvents)();

static _SDL_IME_Init SDL_IME_Init_Real = NULL;
static _SDL_IME_Quit SDL_IME_Quit_Real = NULL;
static _SDL_IME_SetFocus SDL_IME_SetFocus_Real = NULL;
static _SDL_IME_Reset SDL_IME_Reset_Real = NULL;
static _SDL_IME_ProcessKeyEvent SDL_IME_ProcessKeyEvent_Real = NULL;
static _SDL_IME_UpdateTextRect SDL_IME_UpdateTextRect_Real = NULL;
static _SDL_IME_PumpEvents SDL_IME_PumpEvents_Real = NULL;

static void
InitIME()
{
    static bool inited = false;
    const char *im_module = NULL;

    if (inited == true)
        return ;

    inited = true;
    // TODO:
    // better move every ime implenment to a shared library

    // default to IBus
#if 0 //def HAVE_IBUS_IBUS_H
    SDL_IME_Init_Real = SDL_IBus_Init;
    SDL_IME_Quit_Real = SDL_IBus_Quit;
    SDL_IME_SetFocus_Real = SDL_IBus_SetFocus;
    SDL_IME_Reset_Real = SDL_IBus_Reset;
    SDL_IME_ProcessKeyEvent_Real = SDL_IBus_ProcessKeyEvent;
    SDL_IME_UpdateTextRect_Real = SDL_IBus_UpdateTextRect;
    SDL_IME_PumpEvents_Real = SDL_IBus_PumpEvents;
#endif

    //im_module = getenv("SDL_IM_MODULE");
    //if (im_module) {
    //    if (strcmp(im_module, "fcitx") == 0) {
//#ifdef HAVE_FCITX_FRONTEND_H
            SDL_IME_Init_Real = SDL_Fcitx_Init;
            SDL_IME_Quit_Real = SDL_Fcitx_Quit;
            SDL_IME_SetFocus_Real = SDL_Fcitx_SetFocus;
            SDL_IME_Reset_Real = SDL_Fcitx_Reset;
            SDL_IME_ProcessKeyEvent_Real = SDL_Fcitx_ProcessKeyEvent;
            SDL_IME_UpdateTextRect_Real = SDL_Fcitx_UpdateTextRect;
            SDL_IME_PumpEvents_Real = SDL_Fcitx_PumpEvents;
//#endif
    //    }
    //}
}

bool
SDL_IME_Init(void)
{
    InitIME();

    if (SDL_IME_Init_Real)
        return SDL_IME_Init_Real();

    return false;
}

void
SDL_IME_Quit(void)
{
    if (SDL_IME_Quit_Real)
        SDL_IME_Quit_Real();
}

void
SDL_IME_SetFocus(bool focused)
{
    if (SDL_IME_SetFocus_Real)
        SDL_IME_SetFocus_Real(focused);
}

void
SDL_IME_Reset(void)
{
    if (SDL_IME_Reset_Real)
        SDL_IME_Reset_Real();
}

bool
SDL_IME_ProcessKeyEvent(uint32_t keysym, uint32_t keycode)
{
    if (SDL_IME_ProcessKeyEvent_Real)
        return SDL_IME_ProcessKeyEvent_Real(keysym, keycode);

    return false;
}

void
SDL_IME_UpdateTextRect(SDL_Rect *rect)
{
    if (SDL_IME_UpdateTextRect_Real)
        SDL_IME_UpdateTextRect_Real(rect);
}

void
SDL_IME_PumpEvents()
{
    if (SDL_IME_PumpEvents_Real)
        SDL_IME_PumpEvents_Real();
}
