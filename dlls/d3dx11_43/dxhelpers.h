/*
 * Copyright 2024 Connor McAdams for CodeWeavers
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

#include "../d3dx9_36/d3dx_helpers.h"
#define D3DERR_INVALIDCALL 0x8876086c

extern DXGI_FORMAT dxgi_format_from_dds_d3dx_pixel_format_id(enum d3dx_pixel_format_id format);
extern DXGI_FORMAT dxgi_format_from_d3dx_pixel_format_id(enum d3dx_pixel_format_id format);
