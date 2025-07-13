#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

// Static variables for video and graphics
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// New: GX FIFO buffer
#define DEFAULT_FIFO_SIZE	(256*1024)
static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN(32);

// Rectangle properties
float rect_x = 100.0f; // X position of the rectangle
float rect_y = 100.0f; // Y position of the rectangle
float rect_width = 100.0f; // Width of the rectangle
float rect_height = 100.0f; // Height of the rectangle
// u32 rect_color = COLOR_RED; // Old: Now we'll use GXColor directly
GXColor rect_color_gx = (GXColor){255, 0, 0, 255}; // Red (R, G, B, A)
float move_speed = 5.0f; // How many pixels to move per button press

//---------------------------------------------------------------------------------
// Function to setup the GX (Graphics) subsystem
//---------------------------------------------------------------------------------
void setup_gx() {
    f32 yscale;
    u32 xfbHeight;

    // Set up the video registers with the chosen mode (already done in main, but good practice for GX)
    VIDEO_Configure(rmode);

    // Tell the video hardware where our display memory is
    VIDEO_SetNextFramebuffer(xfb);

    // Clear the framebuffer - already done in main, but good for refresh
    // CORRECTED LINE: framebuffer pointer (xfb) is the second argument
    VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);

    // Make the display visible
    VIDEO_SetBlack(false);

    // Flush the video register changes to the hardware
    VIDEO_Flush();

    // Wait for Video setup to complete
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Initialize the GX FIFO and GX
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE); // Pass FIFO buffer and its size

    // Set background color - using GXColor struct now
    GX_SetCopyClear((GXColor){0, 0, 0, 255}, 0x00ffffff); // Black

    // Set up viewport and scissor box (added nearZ and farZ)
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0.0f, 1.0f);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);

    // Calculate Y scale for aspect ratio correction
    yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    xfbHeight = GX_SetDispCopyYScale(yscale);

    // Set display copy attributes
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
    GX_SetDispCopyGamma(GX_GM_1_0); // Set gamma to 1.0 (no gamma correction)

    // Set the texture coordinates, vertex format, and projection matrix
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    // CORRECTED LINE: Added GX_CLR_RGBA as the third argument
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    Mtx44 perspective;
    guOrtho(perspective, 0, rmode->efbHeight, 0, rmode->fbWidth, 0, 1);
    GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

    // Other GX setup for basic 2D drawing
    GX_SetNumChans(1); // One color channel
    GX_SetNumTexGens(0); // No textures
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR); // Pass vertex color directly

    // Corrected GX_SetTevOrder using correct NULL constant names
    GX_SetTevOrder(GX_TEVSTAGE0, GX_COLOR0, GX_TEXCOORDNULL, GX_ALPHA0);

    GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); // No blending
    GX_SetAlphaUpdate(GX_TRUE); // Update alpha
    GX_SetColorUpdate(GX_TRUE); // Update color
    GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE); // No Z-buffer for 2D
}

//---------------------------------------------------------------------------------
// Function to draw a filled rectangle
//---------------------------------------------------------------------------------
// Changed color argument to GXColor
void draw_rectangle(float x, float y, float width, float height, GXColor color) {
    // Begin drawing a quadrilateral (a rectangle)
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    // Vertices for the rectangle
    // Top-left
    GX_Position3f32(x, y, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    // Top-right
    GX_Position3f32(x + width, y, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    // Bottom-right
    GX_Position3f32(x + width, y + height, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    // Bottom-left
    GX_Position3f32(x, y + height, 0);
    GX_Color4u8(color.r, color.g, color.b, color.a);

    // End drawing
    GX_End();
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

    // Initialise the video system
    VIDEO_Init();

    // This function initialises the attached controllers
    WPAD_Init();

    // Set preferred video mode explicitly for composite/480i
    rmode = &TVNtsc480Int; // Assuming NTSC 480i (60Hz) with composite cables

    // Allocate memory for the display in the uncached region
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialise the console, required for printf
    console_init(xfb,20,20,rmode->fbWidth-20,rmode->xfbHeight-20,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    // SYS_STDIO_Report(true); // Uncomment if you want to see printf in Dolphin's console

    // Set up GX for 2D drawing
    setup_gx();

    // The console understands VT terminal escape codes
    printf("\x1b[2;0H");
    printf("Welcome to Roels Wii Channel!\n");
    printf("Press A to move rectangle down, B to move up.\n");
    printf("Press HOME to exit.\n");

    for (int i = 0; i < 180; i++) {
        VIDEO_WaitVSync();
    }

    while(1) {
        // Call WPAD_ScanPads each loop, this reads the latest controller states
        WPAD_ScanPads();

        // WPAD_ButtonsDown tells us which buttons were pressed in this loop (one-shot)
        u32 pressed = WPAD_ButtonsDown(0);

        // Update rectangle position based on button presses
        if (pressed & WPAD_BUTTON_A) {
            rect_y += move_speed;
            // Clamp to bottom of screen (considering rectangle height)
            if (rect_y + rect_height > rmode->efbHeight) {
                rect_y = rmode->efbHeight - rect_height;
            }
        }
        if (pressed & WPAD_BUTTON_B) {
            rect_y -= move_speed;
            // Clamp to top of screen
            if (rect_y < 0) {
                rect_y = 0;
            }
        }

        // We return to the launcher application via exit
        if (pressed & WPAD_BUTTON_HOME) exit(0);

        // --- Graphics Drawing ---
        // These need to be called per frame before drawing
        GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0.0f, 1.0f);
        GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);

        // Clear the screen for the new frame
        GX_SetCopyClear((GXColor){0, 0, 0, 255}, 0x00ffffff); // Clear with black
        GX_CopyDisp(xfb, GX_TRUE); // Copy to the framebuffer

        // Set up the projection again (important if you have multiple drawing passes or complex scenes)
        Mtx44 perspective;
        guOrtho(perspective, 0, rmode->efbHeight, 0, rmode->fbWidth, 0, 1);
        GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

        // Draw our rectangle
        draw_rectangle(rect_x, rect_y, rect_width, rect_height, rect_color_gx);

        // End rendering and flush commands
        GX_DrawDone();

        // Wait for the next frame
        VIDEO_WaitVSync();
    }

    return 0;
}