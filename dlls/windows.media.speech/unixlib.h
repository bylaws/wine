/*
 * Unix library interface for Windows.Media.Speech
 *
 * Copyright 2023 Bernhard Kölbl for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINDOWS_MEDIA_SPEECH_UNIXLIB_H
#define __WINE_WINDOWS_MEDIA_SPEECH_UNIXLIB_H

#include <stdbool.h>
#include <stdint.h>

#include "windef.h"
#include "winternl.h"
#include "wtypes.h"

#include "wine/unixlib.h"

typedef UINT64 speech_recognizer_handle;

struct speech_create_recognizer_params
{
    speech_recognizer_handle handle;
    CHAR locale[LOCALE_NAME_MAX_LENGTH];
    FLOAT sample_rate;
};

struct speech_release_recognizer_params
{
    speech_recognizer_handle handle;
};

enum unix_funcs
{
    unix_process_attach,
    unix_process_detach,
    unix_speech_create_recognizer,
    unix_speech_release_recognizer,
};

#endif
