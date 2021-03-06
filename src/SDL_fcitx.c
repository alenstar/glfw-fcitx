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

#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <fcitx/frontend.h>
#include "def.h"
#include "SDL_fcitx.h"
#include <SDL2/SDL.h>
#include "SDL_dbus.h"
#include <SDL2/SDL_syswm.h>
#if SDL_VIDEO_DRIVER_X11
//#  include <SDL2/SDL_x11video.h>
#endif
#include <SDL2/SDL_hints.h>

#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx"

#define FCITX_IM_DBUS_PATH "/inputmethod"
#define FCITX_IC_DBUS_PATH "/inputcontext_%d"

#define FCITX_IM_DBUS_INTERFACE "org.fcitx.Fcitx.InputMethod"
#define FCITX_IC_DBUS_INTERFACE "org.fcitx.Fcitx.InputContext"

#define IC_NAME_MAX 64
#define DBUS_TIMEOUT 500

#define UTF8_IsLeadByte(c) ((c) >= 0xC0 && (c) <= 0xF4)
#define UTF8_IsTrailingByte(c) ((c) >= 0x80 && (c) <= 0xBF)
/* !!! FIXME: these have side effects. You probably shouldn't use them. */
/* !!! FIXME: Maybe we do forceinline functions of SDL_mini, SDL_minf, etc? */
#define SDL_min(x, y) (((x) < (y)) ? (x) : (y))
#define SDL_max(x, y) (((x) > (y)) ? (x) : (y))

static int UTF8_TrailingBytes(unsigned char c)
{
    if (c >= 0xC0 && c <= 0xDF)
        return 1;
    else if (c >= 0xE0 && c <= 0xEF)
        return 2;
    else if (c >= 0xF0 && c <= 0xF4)
        return 3;
    else
        return 0;
}

size_t SDL_utf8strlcpy(char *dst, const char *src, size_t dst_bytes)
{
    size_t src_bytes = strlen(src);
    size_t bytes = SDL_min(src_bytes, dst_bytes - 1);
    size_t i = 0;
    char trailing_bytes = 0;
    if (bytes)
    {
        unsigned char c = (unsigned char)src[bytes - 1];
        if (UTF8_IsLeadByte(c))
            --bytes;
        else if (UTF8_IsTrailingByte(c))
        {
            for (i = bytes - 1; i != 0; --i)
            {
                c = (unsigned char)src[i];
                trailing_bytes = UTF8_TrailingBytes(c);
                if (trailing_bytes)
                {
                    if (bytes - i != trailing_bytes + 1)
                        bytes = i;

                    break;
                }
            }
        }
        memcpy(dst, src, bytes);
    }
    dst[bytes] = '\0';
    return bytes;
}

#if 0
int
SDL_SendKeyboardText(const char *text)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;
    int posted;

    /* Don't post text events for unprintable characters */
    if ((unsigned char)*text < ' ' || *text == 127) {
        return 0;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_TEXTINPUT) == SDL_ENABLE) {
        SDL_Event event;
        event.text.type = SDL_TEXTINPUT;
        event.text.windowID = keyboard->focus ? keyboard->focus->id : 0;
        SDL_utf8strlcpy(event.text.text, text, SDL_arraysize(event.text.text));
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

int
SDL_SendEditingText(const char *text, int start, int length)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;
    int posted;

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_TEXTEDITING) == SDL_ENABLE) {
        SDL_Event event;
        event.edit.type = SDL_TEXTEDITING;
        event.edit.windowID = keyboard->focus ? keyboard->focus->id : 0;
        event.edit.start = start;
        event.edit.length = length;
        SDL_utf8strlcpy(event.edit.text, text, SDL_arraysize(event.edit.text));
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}
#endif 

typedef struct _FcitxClient
{
    SDL_DBusContext *dbus;

    char servicename[IC_NAME_MAX];
    char icname[IC_NAME_MAX];

    int id;

    SDL_Rect cursor_rect;
} FcitxClient;

static FcitxClient fcitx_client;

static int
GetDisplayNumber()
{
    const char *display = getenv("DISPLAY");
    const char *p = NULL;;
    int number = 0;

    if (display == NULL)
        return 0;

    display = strchr(display, ':');
    if (display == NULL)
        return 0;

    display++;
    p = strchr(display, '.');
    if (p == NULL && display != NULL) {
        number = strtod(display, NULL);
    } else {
        char *buffer = strdup(display);
        buffer[p - display] = '\0';
        number = strtod(buffer, NULL);
        free(buffer);
    }

    return number;
}

static char*
GetAppName()
{
#if defined(__LINUX__) || defined(__FREEBSD__)
    char *spot;
    char procfile[1024];
    char linkfile[1024];
    int linksize;

#if defined(__LINUX__)
    snprintf(procfile, sizeof(procfile), "/proc/%d/exe", getpid());
#elif defined(__FREEBSD__)
    snprintf(procfile, sizeof(procfile), "/proc/%d/file", getpid());
#endif
    linksize = readlink(procfile, linkfile, sizeof(linkfile) - 1);
    if (linksize > 0) {
        linkfile[linksize] = '\0';
        spot = strrchr(linkfile, '/');
        if (spot) {
            return strdup(spot + 1);
        } else {
            return strdup(linkfile);
        }
    }
#endif /* __LINUX__ || __FREEBSD__ */

    return strdup("SDL_App");
}

/*
 * Copied from fcitx source
 */
#define CONT(i)   ISUTF8_CB(in[i])
#define VAL(i, s) ((in[i]&0x3f) << s)

static char *
_fcitx_utf8_get_char(const char *i, uint32_t *chr)
{
    const unsigned char* in = (const unsigned char *)i;
    if (!(in[0] & 0x80)) {
        *(chr) = *(in);
        return (char *)in + 1;
    }

    /* 2-byte, 0x80-0x7ff */
    if ((in[0] & 0xe0) == 0xc0 && CONT(1)) {
        *chr = ((in[0] & 0x1f) << 6) | VAL(1, 0);
        return (char *)in + 2;
    }

    /* 3-byte, 0x800-0xffff */
    if ((in[0] & 0xf0) == 0xe0 && CONT(1) && CONT(2)) {
        *chr = ((in[0] & 0xf) << 12) | VAL(1, 6) | VAL(2, 0);
        return (char *)in + 3;
    }

    /* 4-byte, 0x10000-0x1FFFFF */
    if ((in[0] & 0xf8) == 0xf0 && CONT(1) && CONT(2) && CONT(3)) {
        *chr = ((in[0] & 0x7) << 18) | VAL(1, 12) | VAL(2, 6) | VAL(3, 0);
        return (char *)in + 4;
    }

    /* 5-byte, 0x200000-0x3FFFFFF */
    if ((in[0] & 0xfc) == 0xf8 && CONT(1) && CONT(2) && CONT(3) && CONT(4)) {
        *chr = ((in[0] & 0x3) << 24) | VAL(1, 18) | VAL(2, 12) | VAL(3, 6) | VAL(4, 0);
        return (char *)in + 5;
    }

    /* 6-byte, 0x400000-0x7FFFFFF */
    if ((in[0] & 0xfe) == 0xfc && CONT(1) && CONT(2) && CONT(3) && CONT(4) && CONT(5)) {
        *chr = ((in[0] & 0x1) << 30) | VAL(1, 24) | VAL(2, 18) | VAL(3, 12) | VAL(4, 6) | VAL(5, 0);
        return (char *)in + 6;
    }

    *chr = *in;

    return (char *)in + 1;
}

static size_t
_fcitx_utf8_strlen(const char *s)
{
    unsigned int l = 0;

    while (*s) {
        uint32_t chr;

        s = _fcitx_utf8_get_char(s, &chr);
        l++;
    }

    return l;
}

static DBusHandlerResult
DBus_MessageFilter(DBusConnection *conn, DBusMessage *msg, void *data)
{
    SDL_DBusContext *dbus = (SDL_DBusContext *)data;

    if (dbus->message_is_signal(msg, FCITX_IC_DBUS_INTERFACE, "CommitString")) {
        DBusMessageIter iter;
        const char *text = NULL;

        dbus->message_iter_init(msg, &iter);
        dbus->message_iter_get_basic(&iter, &text);

        if (text) {
		// TODO 
        //    SDL_SendKeyboardText(text);
			LOGD("Text:%s ", text);
		}

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (dbus->message_is_signal(msg, FCITX_IC_DBUS_INTERFACE, "UpdatePreedit")) {
        DBusMessageIter iter;
        const char *text;

        dbus->message_iter_init(msg, &iter);
        dbus->message_iter_get_basic(&iter, &text);

        if (text && *text) {
            char buf[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
            size_t text_bytes = strlen(text), i = 0;
            size_t cursor = 0;

            while (i < text_bytes) {
				// TODO
                size_t sz = SDL_utf8strlcpy(buf, text + i, sizeof(buf));
                size_t chars = _fcitx_utf8_strlen(buf);

				// TODO
                //SDL_SendEditingText(buf, cursor, chars);
				LOGD("EditingText:%s %d %d ", buf, cursor, chars);
                i += sz;
                cursor += chars;
            }
        }

        SDL_Fcitx_UpdateTextRect(NULL);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusMessage*
FcitxClientICNewMethod(FcitxClient *client,
        const char *method)
{
    SDL_DBusContext *dbus = client->dbus;
    return dbus->message_new_method_call(
            client->servicename,
            client->icname,
            FCITX_IC_DBUS_INTERFACE,
            method);
}

static void
FcitxClientICCallMethod(FcitxClient *client,
        const char *method)
{
    SDL_DBusContext *dbus = client->dbus;
    DBusMessage *msg = FcitxClientICNewMethod(client, method);

    if (msg == NULL)
        return ;

    if (dbus->connection_send(dbus->session_conn, msg, NULL)) {
        dbus->connection_flush(dbus->session_conn);
    }

    dbus->message_unref(msg);
}

static void
Fcitx_SetCapabilities(void *data,
        const char *name,
        const char *old_val,
        const char *internal_editing)
{
    FcitxClient *client = (FcitxClient *)data;
    SDL_DBusContext *dbus = client->dbus;
    Uint32 caps = CAPACITY_NONE;

    DBusMessage *msg = FcitxClientICNewMethod(client, "SetCapacity");
    if (msg == NULL)
        return ;

    if (!(internal_editing && *internal_editing == '1')) {
        caps |= CAPACITY_PREEDIT;
    }

    dbus->message_append_args(msg,
            DBUS_TYPE_UINT32, &caps,
            DBUS_TYPE_INVALID);
    if (dbus->connection_send(dbus->session_conn, msg, NULL)) {
        dbus->connection_flush(dbus->session_conn);
    }

    dbus->message_unref(msg);
}

static void
FcitxClientCreateIC(FcitxClient *client)
{
    char *appname = NULL;
    pid_t pid = 0;
    int id = 0;
    SDL_bool enable;
    Uint32 arg1, arg2, arg3, arg4;

    SDL_DBusContext *dbus = client->dbus;
    DBusMessage *reply = NULL;
    DBusMessage *msg = dbus->message_new_method_call(
            client->servicename,
            FCITX_IM_DBUS_PATH,
            FCITX_IM_DBUS_INTERFACE,
            "CreateICv3"
            );

    if (msg == NULL)
        return ;

    appname = GetAppName();
    pid = getpid();
    dbus->message_append_args(msg,
            DBUS_TYPE_STRING, &appname,
            DBUS_TYPE_INT32, &pid,
            DBUS_TYPE_INVALID);

    do {
        reply = dbus->connection_send_with_reply_and_block(
                dbus->session_conn,
                msg,
                DBUS_TIMEOUT,
                NULL);

        if (!reply)
            break;
        if (!dbus->message_get_args(reply, NULL,
                DBUS_TYPE_INT32, &id,
                DBUS_TYPE_BOOLEAN, &enable,
                DBUS_TYPE_UINT32, &arg1,
                DBUS_TYPE_UINT32, &arg2,
                DBUS_TYPE_UINT32, &arg3,
                DBUS_TYPE_UINT32, &arg4,
                DBUS_TYPE_INVALID))
            break;

        if (id < 0)
            break;
        client->id = id;

        snprintf(client->icname, IC_NAME_MAX,
                FCITX_IC_DBUS_PATH, client->id);

        dbus->bus_add_match(dbus->session_conn,
                "type='signal', interface='org.fcitx.Fcitx.InputContext'",
                NULL);
        dbus->connection_add_filter(dbus->session_conn,
                &DBus_MessageFilter, dbus,
                NULL);
        dbus->connection_flush(dbus->session_conn);

		// TODO
        // SDL_AddHintCallback(SDL_HINT_IME_INTERNAL_EDITING, &Fcitx_SetCapabilities, client);
    }
    while (0);

    if (reply)
        dbus->message_unref(reply);
    dbus->message_unref(msg);
    free(appname);
}

static Uint32
Fcitx_ModState(void)
{
    uint32_t fcitx_mods = 0;
	// TODO
    SDL_Keymod sdl_mods = 0; //SDL_GetModState();

    if (sdl_mods & KMOD_SHIFT) fcitx_mods |= FcitxKeyState_Shift;
    if (sdl_mods & KMOD_CAPS)   fcitx_mods |= FcitxKeyState_CapsLock;
    if (sdl_mods & KMOD_CTRL)  fcitx_mods |= FcitxKeyState_Ctrl;
    if (sdl_mods & KMOD_ALT)   fcitx_mods |= FcitxKeyState_Alt;
    if (sdl_mods & KMOD_NUM)    fcitx_mods |= FcitxKeyState_NumLock;
    if (sdl_mods & KMOD_LGUI)   fcitx_mods |= FcitxKeyState_Super;
    if (sdl_mods & KMOD_RGUI)   fcitx_mods |= FcitxKeyState_Meta;

    return fcitx_mods;
}

bool
SDL_Fcitx_Init()
{
    fcitx_client.dbus = SDL_DBus_GetContext();

    fcitx_client.cursor_rect.x = -1;
    fcitx_client.cursor_rect.y = -1;
    fcitx_client.cursor_rect.w = 0;
    fcitx_client.cursor_rect.h = 0;

    snprintf(fcitx_client.servicename, IC_NAME_MAX,
            "%s-%d",
            FCITX_DBUS_SERVICE, GetDisplayNumber());

    FcitxClientCreateIC(&fcitx_client);

    return true;
}

void
SDL_Fcitx_Quit()
{
    FcitxClientICCallMethod(&fcitx_client, "DestroyIC");
}

void
SDL_Fcitx_SetFocus(bool focused)
{
    if (focused) {
        FcitxClientICCallMethod(&fcitx_client, "FocusIn");
    } else {
        FcitxClientICCallMethod(&fcitx_client, "FocusOut");
    }
}

void
SDL_Fcitx_Reset(void)
{
    FcitxClientICCallMethod(&fcitx_client, "Reset");
    FcitxClientICCallMethod(&fcitx_client, "CloseIC");
}

bool
SDL_Fcitx_ProcessKeyEvent(uint32_t keysym, uint32_t keycode)
{
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    SDL_DBusContext *dbus = fcitx_client.dbus;

    Uint32 state = 0;
    bool handled = false;
    int type = FCITX_PRESS_KEY;
    Uint32 event_time = 0;

    msg = FcitxClientICNewMethod(&fcitx_client, "ProcessKeyEvent");
    if (msg == NULL) {
		LOGD("NewMethod ProcessKeyEvent");
        return false;
	}

    state = Fcitx_ModState();
    dbus->message_append_args(msg,
            DBUS_TYPE_UINT32, &keysym,
            DBUS_TYPE_UINT32, &keycode,
            DBUS_TYPE_UINT32, &state,
            DBUS_TYPE_INT32, &type,
            DBUS_TYPE_UINT32, &event_time,
            DBUS_TYPE_INVALID);

    reply = dbus->connection_send_with_reply_and_block(dbus->session_conn,
            msg,
            -1,
            NULL);

    if (reply) {
        dbus->message_get_args(reply,
                NULL,
                DBUS_TYPE_INT32, &handled,
                DBUS_TYPE_INVALID);

        dbus->message_unref(reply);
    }

    if (handled) {
		// TODO
        // SDL_Fcitx_UpdateTextRect(NULL);
		LOGD("UpdateTextRect");
    }

    return handled;
}

void
SDL_Fcitx_UpdateTextRect(SDL_Rect *rect)
{
    int x = 0, y = 0;
	// TODO
#if 0
    SDL_Window *focused_win = NULL;
    SDL_SysWMinfo info;
    SDL_Rect *cursor = &fcitx_client.cursor_rect;

    SDL_DBusContext *dbus = fcitx_client.dbus;
    DBusMessage *msg = NULL;
    DBusConnection *conn;

    if (rect) {
        SDL_memcpy(cursor, rect, sizeof(SDL_Rect));
    }

    focused_win = SDL_GetKeyboardFocus();
    if (!focused_win) {
        return ;
    }

    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(focused_win, &info)) {
        return;
    }

    SDL_GetWindowPosition(focused_win, &x, &y);

#if SDL_VIDEO_DRIVER_X11
    if (info.subsystem == SDL_SYSWM_X11) {
        SDL_DisplayData *displaydata = (SDL_DisplayData *) SDL_GetDisplayForWindow(focused_win)->driverdata;

        Display *x_disp = info.info.x11.display;
        Window x_win = info.info.x11.window;
        int x_screen = displaydata->screen;
        Window unused;
        X11_XTranslateCoordinates(x_disp, x_win, RootWindow(x_disp, x_screen), 0, 0, &x, &y, &unused);
    }
#endif

    if (cursor->x == -1 && cursor->y == -1 && cursor->w == 0 && cursor->h == 0) {
        // move to bottom left
        int w = 0, h = 0;
        SDL_GetWindowSize(focused_win, &w, &h);
        cursor->x = 0;
        cursor->y = h;
    }

    x += cursor->x;
    y += cursor->y;

	// 
    msg = FcitxClientICNewMethod(&fcitx_client, "SetCursorRect");
    if (msg == NULL)
        return ;

    dbus->message_append_args(msg,
            DBUS_TYPE_INT32, &x,
            DBUS_TYPE_INT32, &y,
            DBUS_TYPE_INT32, &cursor->w,
            DBUS_TYPE_INT32, &cursor->h,
            DBUS_TYPE_INVALID);

    conn = dbus->session_conn;
    if (dbus->connection_send(conn, msg, NULL))
        dbus->connection_flush(conn);

    dbus->message_unref(msg);
#endif
}

void
SDL_Fcitx_PumpEvents()
{
    SDL_DBusContext *dbus = fcitx_client.dbus;
    DBusConnection *conn = dbus->session_conn;

    dbus->connection_read_write(conn, 0);

    while (dbus->connection_dispatch(conn) == DBUS_DISPATCH_DATA_REMAINS) {
        /* Do nothing, actual work happens in DBus_MessageFilter */
        usleep(10);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
