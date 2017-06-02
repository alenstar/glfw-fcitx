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

#ifndef _SDL_ime_h
#define _SDL_ime_h

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_rect.h>

extern bool SDL_IME_Init();
extern void SDL_IME_Quit();
extern void SDL_IME_SetFocus(bool focused);
extern void SDL_IME_Reset();
extern bool SDL_IME_ProcessKeyEvent(uint32_t keysym, uint32_t keycode);
extern void SDL_IME_UpdateTextRect(SDL_Rect *rect);
extern void SDL_IME_PumpEvents();

#endif /* _SDL_ime_h */
