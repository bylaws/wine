/* MESA private include file
   (c) 1998 Lionel ULMER
   
   This files contains all the structure that are not exported
   through d3d.h and all common macros. */

#ifndef __GRAPHICS_WINE_MESA_PRIVATE_H
#define __GRAPHICS_WINE_MESA_PRIVATE_H

#include "d3d_private.h"

#include "wine_gl.h"
#include "x11drv.h"

#undef USE_OSMESA

#include "x11_private.h"

/*****************************************************************************
 * IDirect3DLight MESA private structure
 */
typedef struct mesa_d3dl_private
{
    GLenum              light_num;
} mesa_d3dl_private;

/*****************************************************************************
 * IDirect3DTexture2 MESA private structure
 */
typedef struct mesa_d3dt_private
{
    GLuint                   tex_name;
} mesa_d3dt_private;

/*****************************************************************************
 * IDirect3DViewport2 implementation structure
 */
typedef struct mesa_d3dv_private
{
    GLenum                      nextlight;
} mesa_d3dv_private;

/* Matrix copy WITH transposition */
#define conv_mat2(mat,gl_mat)			\
{						\
    TRACE("%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
    TRACE("%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
    TRACE("%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
    TRACE("%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44); \
    (gl_mat)[ 0] = (mat)->_11;			\
    (gl_mat)[ 1] = (mat)->_21;			\
    (gl_mat)[ 2] = (mat)->_31;			\
    (gl_mat)[ 3] = (mat)->_41;			\
    (gl_mat)[ 4] = (mat)->_12;			\
    (gl_mat)[ 5] = (mat)->_22;			\
    (gl_mat)[ 6] = (mat)->_32;			\
    (gl_mat)[ 7] = (mat)->_42;			\
    (gl_mat)[ 8] = (mat)->_13;			\
    (gl_mat)[ 9] = (mat)->_23;			\
    (gl_mat)[10] = (mat)->_33;			\
    (gl_mat)[11] = (mat)->_43;			\
    (gl_mat)[12] = (mat)->_14;			\
    (gl_mat)[13] = (mat)->_24;			\
    (gl_mat)[14] = (mat)->_34;			\
    (gl_mat)[15] = (mat)->_44;			\
};

/* Matrix copy WITHOUT transposition */
#define conv_mat(mat,gl_mat)			\
{                                               \
    TRACE("%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
    TRACE("%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
    TRACE("%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
    TRACE("%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44); \
    memcpy(gl_mat, (mat), 16 * sizeof(float));      \
};

typedef struct render_state {
  /* This is used for the device mode */
  GLenum src, dst;
  /* This is used for textures */
  GLenum mag, min;
} RenderState;

typedef struct mesa_d3dd_private {
#ifdef USE_OSMESA
    OSMesaContext ctx;
    unsigned char *buffer;
#else
    GLXContext ctx;
#endif

    /* The current render state */
    RenderState rs;

    /* The last type of vertex drawn */
    D3DVERTEXTYPE vt;

    D3DMATRIX *world_mat;
    D3DMATRIX *view_mat;
    D3DMATRIX *proj_mat;
} mesa_d3dd_private;

#define _dump_colorvalue(s,v)               \
    TRACE(" " s " : %f %f %f %f\n",           \
	    (v).r.r, (v).g.g, (v).b.b, (v).a.a);

/* Common functions defined in d3dcommon.c */
void set_render_state(D3DRENDERSTATETYPE dwRenderStateType,
		      DWORD dwRenderState, RenderState *rs) ;

/* All non-static functions 'exported' by various sub-objects */
extern LPDIRECT3DTEXTURE2 d3dtexture2_create(IDirectDrawSurface4Impl* surf);
extern LPDIRECT3DTEXTURE d3dtexture_create(IDirectDrawSurface4Impl* surf);

extern LPDIRECT3DLIGHT d3dlight_create_dx3(IDirect3DImpl* d3d1);
extern LPDIRECT3DLIGHT d3dlight_create(IDirect3D2Impl* d3d2);

extern LPDIRECT3DEXECUTEBUFFER d3dexecutebuffer_create(IDirect3DDeviceImpl* d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc);

extern LPDIRECT3DMATERIAL d3dmaterial_create(IDirect3DImpl* d3d1);
extern LPDIRECT3DMATERIAL2 d3dmaterial2_create(IDirect3D2Impl* d3d2);

extern LPDIRECT3DVIEWPORT d3dviewport_create(IDirect3DImpl* d3d1);
extern LPDIRECT3DVIEWPORT2 d3dviewport2_create(IDirect3D2Impl* d3d2);

extern int is_OpenGL_dx3(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDeviceImpl** device);
extern int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int is_OpenGL(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDevice2Impl** device, IDirect3D2Impl* d3d);

extern LPDIRECT3DTEXTURE2 mesa_d3dtexture2_create(IDirectDrawSurface4Impl*);
extern LPDIRECT3DTEXTURE mesa_d3dtexture_create(IDirectDrawSurface4Impl*);

static const GUID WINE_UNUSED IID_D3DDEVICE2_OpenGL = {
  0x39a0da38,
  0x7e57,
  0x11d2,
  { 0x8b,0x7c,0x0e,0x4e,0xd8,0x3c,0x2b,0x3c }
};

static const GUID WINE_UNUSED IID_D3DDEVICE_OpenGL = {
  0x31416d44,
  0x86ae,
  0x11d2,
  { 0x82,0x2d,0xa8,0xd5,0x31,0x87,0xca,0xfa }
};

extern ICOM_VTABLE(IDirect3D) mesa_d3dvt;
extern ICOM_VTABLE(IDirect3D2) mesa_d3d2vt;
#endif /* __GRAPHICS_WINE_MESA_PRIVATE_H */
