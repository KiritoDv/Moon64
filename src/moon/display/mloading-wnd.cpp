#include "mloading-wnd.h"

#include "moon/mod-engine/textures/assets/l-moon.h"
#include "moon/mod-engine/textures/modifiers/animated.h"

#include <iostream>
#include <assert.h>
#include <unistd.h>
#include <thread>
#include <string>

#include <SDL2/SDL.h>
#ifndef _WIN32
#include <SDL2/SDL_ttf.h>
#endif
#include <SDL2/SDL_image.h>

#include "umath.h"

using namespace std;
bool isWndRunning = false;
SDL_Window *wnd;
SDL_Renderer *renderer;
#ifndef _WIN32
TTF_Font* font;
#endif

SDL_Texture* background;
SDL_Texture* logo;
SDL_Texture* message;

string textMessage = "Moon64 Loading";

int maxAddons = 0;
float loadedAddons = 0;
int globalTime = 0;
bool raised = false;

#define WND_WIDTH  256
#define WND_HEIGHT 256

namespace MoonWindow {

    void setupWindow(){
        isWndRunning = true;
        SDL_Init(SDL_INIT_EVERYTHING);
    #ifndef _WIN32
        TTF_Init();
        font = TTF_OpenFont("/home/alex/disks/uwu/Projects/UnderVolt/Moon64/scientificaBold.ttf", 18);
        if( font == NULL ) {
            cout << TTF_GetError() << endl;
        }
    #endif
        wnd = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256, 256, SDL_WINDOW_BORDERLESS);
        if(!wnd){
            SDL_DestroyWindow(wnd);
            return;
        }
        renderer = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

        background = SDL_CreateTextureFromSurface(renderer, SDL_CreateRGBSurfaceFrom(
            (void*) loading_bg.pixel_data,
            loading_bg.width,
            loading_bg.height,
            32,
            4 * loading_bg.width,
            0x000000ff,
            0x0000ff00,
            0x00ff0000,
            0xff000000
        ));

        logo = SDL_CreateTextureFromSurface(renderer, SDL_CreateRGBSurfaceFrom(
            (void*) loading_moon.pixel_data,
            loading_moon.width,
            loading_moon.height,
            32,
            4 * loading_moon.width,
            0x000000ff,
            0x0000ff00,
            0x00ff0000,
            0xff000000
        ));
    }

    void updateMessage(string msg){
        textMessage = msg;
    }

    int count = 0;
    float tmp = 0;
    float yLogo;

    void tickDraw(double deltaTime){

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        if(count < 30) count++;
        else count = 0;

        yLogo = MathUtil::Lerp(yLogo, MathUtil::PingPong(globalTime, 60), 0.1f * deltaTime);

        // Draw Background
        SDL_Rect backgroundRect = {.x = 0, .y = 0, .w = 256, .h = 256};
        SDL_RenderCopy(renderer, background, NULL, &backgroundRect);

        // Draw Logo
        SDL_FRect logoRect = {.x = 0, .y = 20 - yLogo, .w = 256, .h = 256};
        SDL_RenderCopyF(renderer, logo, NULL, &logoRect);
    #ifndef _WIN32
        // Draw text
        SDL_Surface *message = TTF_RenderText_Solid(font, textMessage.c_str(), {255, 255, 255});
        SDL_Texture *msg = SDL_CreateTextureFromSurface(renderer, message);
        SDL_Rect textRect = {.x = (WND_WIDTH / 2) - (message->w / 2), .y = WND_HEIGHT - 15 - message->h, .w = message->w, .h = message->h};
        SDL_RenderCopy(renderer, msg, NULL, new SDL_Rect());
        SDL_FreeSurface(message);
        SDL_DestroyTexture(msg);
    #endif
        // Draw progress bar
        SDL_SetRenderDrawColor(renderer, 255, 239, 97, 255);
        SDL_FRect pbRect = {.x = 0, .y = WND_HEIGHT - 5, .w = WND_WIDTH * (loadedAddons / (float) maxAddons), .h = 5};
        SDL_RenderFillRectF(renderer, &pbRect);

        SDL_RenderPresent(renderer);

        if(!raised){
            SDL_RaiseWindow(wnd);
            raised = true;
        }
    }

    void setMaxAddons(int max){
        maxAddons = max;
    }
    void addAddonCounter(){
        loadedAddons++;
    }

    void setupLoadingWindow(string status){
        if(status == "PreStartup"){
            setupWindow();
            std::thread tickLoop([](){
                SDL_Event e;
                Uint32 last = SDL_GetTicks();
                int counter = 0;
                while(isWndRunning){
                    Uint32 now = SDL_GetTicks();
                    globalTime++;

                    while( SDL_PollEvent( &e ) != 0 ) {
                        switch(e.type){
                            case SDL_QUIT:
                                isWndRunning = false;
                                break;
                        }
                    }
                    MoonWindow::tickDraw((now - last) / 100.0f);
                    last = now;
                }
            });
            tickLoop.detach();
            return;
        }
        if(status == "PostInit"){
            MoonWindow::updateMessage("Finishing loading");
            sleep(2);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(wnd);
        }
        if(status == "Exit"){
            isWndRunning = false;
        }
    }
}