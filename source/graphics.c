#include <gccore.h>
#include <wiiuse/wpad.h>

// Rectangle properties
float rect_x = 100.0f; // X position of the rectangle
float rect_y = 100.0f; // Y position of the rectangle
float rect_width = 100.0f; // Width of the rectangle
float rect_height = 100.0f; // Height of the rectangle
GXColor rect_color_gx = (GXColor){255, 0, 0, 255}; // Red (R, G, B, A)
float move_speed = 5.0f; // How many pixels to move per button press

#define DEFAULT_FIFO_SIZE (256*1024)
static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN(32);

void setup_gx(void *xfb, GXRModeObj *rmode) {
    f32 yscale;
    u32 xfbHeight;
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    GX_SetCopyClear((GXColor){0, 0, 0, 255}, 0x00ffffff);
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0.0f, 1.0f);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
    GX_SetDispCopyGamma(GX_GM_1_0);
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    Mtx44 perspective;
    guOrtho(perspective, 0, rmode->efbHeight, 0, rmode->fbWidth, 0, 1);
    GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);
    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_COLOR0, GX_TEXCOORDNULL, GX_ALPHA0);
    GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
}

void draw_rectangle(float x, float y, float width, float height, GXColor color) {
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(x, y, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_Position3f32(x + width, y, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_Position3f32(x + width, y + height, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_Position3f32(x, y + height, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);
    GX_End();
}
