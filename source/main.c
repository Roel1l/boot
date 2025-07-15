#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>
#include <string.h>

#include <ogc/lwp.h>
#include "mqtt_client.h"
// Global running flag for thread control
volatile bool running = true;

// Thread handle and stack for MQTT
static lwp_t mqtt_thread = (lwp_t)NULL;
#define MQTT_THREAD_STACKSIZE 8192
static u8 mqtt_thread_stack[MQTT_THREAD_STACKSIZE] ATTRIBUTE_ALIGN(32);

// Thread wrapper for mqtt_receive_loop
void* mqtt_thread_func(void* arg) {
    mqtt_receive_loop();
    return NULL;
}

// Static variables for video and graphics
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
int main(int argc, char **argv) {

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

    // The console understands VT terminal escape codes
    printf("\x1b[2;0H");
    printf("Welcome to Roels Wii Channel!\n");
    printf("Press HOME to exit.\n");

    for (int i = 0; i < 180; i++) {
        VIDEO_WaitVSync();
    }

    // Initialize network subsystem just before MQTT
    if (net_init() < 0) {
        printf("Network init failed\n");
        return 1;
    }
    printf("Network initialized\n");

    // Simple TCP connect-based ping to test.mosquitto.org
    {
        int sock;
        struct sockaddr_in addr;
        sock = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0) {
            printf("Ping: socket creation failed\n");
        } else {
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(1883);
            addr.sin_addr.s_addr = inet_addr("5.196.78.28");
            printf("Ping: attempting to connect to test.mosquitto.org...\n");
            if (net_connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                printf("Ping: test.mosquitto.org is reachable!\n");
                net_close(sock);
            } else {
                printf("Ping: test.mosquitto.org is NOT reachable!\n");
                net_close(sock);
            }
        }
    }


    // Start MQTT receive loop in a separate thread
    LWP_CreateThread(&mqtt_thread, mqtt_thread_func, NULL, mqtt_thread_stack, MQTT_THREAD_STACKSIZE, 50);

    // Main loop: poll for HOME button and exit cleanly
    while(running) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) {
            printf("HOME button pressed. Shutting down project...\n");
            running = false;
            break;
        }
        VIDEO_WaitVSync();
    }

    // Wait for MQTT thread to finish
    void* thread_ret;
    LWP_JoinThread(mqtt_thread, &thread_ret);

    return 0;
}