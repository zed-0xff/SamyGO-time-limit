#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

extern unsigned int _ZN8SsObject14m_poObjectListE[];
extern int _ZN14SsKeyInputBase7SendKeyEi(unsigned int, int);

#define SCREEN_W	(1920/2)
#define SCREEN_H	(1080/2)
#define SCREEN_BPP	32
#define SCREEN_FLAGS	SDL_HWSURFACE

#define TTF_FONT   "/mtd_appdata/Font/shadow.ttf"
#define TTF_SIZE   24

#define DAILY_ALLOWED_TIME      (3600*7/2) // 3.5 hours (in seconds)
#define COUNTER_WRITE_PERIOD    (60*2)     // 2 minutes (in seconds)

#define COUNTER_FILE      "/mtd_rwarea/time_limit.dat"

#define PADDING 2 // time box padding in pixels

//#define INCLUDE_SECONDS // include seconds in on-screen counter

int read_counter(int current_day_idx){
    FILE *f = fopen(COUNTER_FILE, "rb");
    if( !f ) {
        return DAILY_ALLOWED_TIME;
    }

    int saved_day_idx = 0;
    fread(&saved_day_idx, sizeof(saved_day_idx), 1, f);
    
    if( saved_day_idx != current_day_idx ){
        fclose(f);
        return DAILY_ALLOWED_TIME;
    }

    int remaining_time = 0;
    fread(&remaining_time, sizeof(remaining_time), 1, f);
    fclose(f);
    return remaining_time;
}

void write_counter(int current_day_idx, int remaining_time){
    FILE *f = fopen(COUNTER_FILE, "wb");
    if( !f ) {
        FILE* log = fopen( "/mtd_ram/time_limit_error.log", "a+" );
        fprintf(log, "Cannot open counter flush file \"%s\" !\n", COUNTER_FILE);
        fclose(log);
        return;
    }

    fwrite(&current_day_idx, sizeof(current_day_idx), 1, f);
    fwrite(&remaining_time,  sizeof(remaining_time),  1, f);
    fclose(f);
    sync();
}

void _Z10OSPowerOffv();

void poweroff(){
    sync();
    SDL_Delay(500);
    _Z10OSPowerOffv();
}

void time_limit_thread(void *tt){
    char buf[0x100];
    SDL_Surface* screen, *text;
    TTF_Font* ttf_font;
    int current_time = (int)tt;
    int current_day_idx = current_time/86400; // 86400 seconds in one day
    int remaining_time = read_counter(current_day_idx);
    FILE* log = fopen( "/mtd_ram/time_limit.log", "a+" );

    SDL_Delay(1000);

    screen = SDL_GetVideoSurface();

    if (screen) {
            SDL_FreeSurface(screen);
            SDL_Quit();
            SDL_Init(SDL_INIT_VIDEO);
            SDL_SetVideoMode(SCREEN_W, SCREEN_H, SCREEN_BPP, SCREEN_FLAGS);
            screen = SDL_GetVideoSurface();
            SDL_Flip(screen);
    } else {
            screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, SCREEN_BPP, SCREEN_FLAGS);
    }

    fprintf(log, "screen = %p, current_day_idx=%d, current_time=%d, remaining_time=%d\n", 
            screen, current_day_idx, current_time, remaining_time
            ); 
    fflush(log);

    if (TTF_Init()!=0){
        fprintf(log, "TTF_Init() FAIL!\n");
        fclose(log);
        return;
    }

    if(NULL == (ttf_font = TTF_OpenFont(TTF_FONT, TTF_SIZE))){
        fprintf(log, "TTF_OpenFont(\"%s\",%d) FAIL!\n", TTF_FONT, TTF_SIZE);
        fclose(log);
        return;
    }

    int bw, bh;
#ifdef INCLUDE_SECONDS
    if( -1 == TTF_SizeUTF8(ttf_font, "8:88:88", &bw, &bh)){
#else
    if( -1 == TTF_SizeUTF8(ttf_font, "8:88", &bw, &bh)){
#endif
        fprintf(log, "TTF_SizeUTF8() FAIL!\n");
        fclose(log);
        return;
    }
    SDL_Rect box;
    box.w = bw+PADDING*2;
    box.h = bh+PADDING*2;
    box.x = SCREEN_W-box.w;
    box.y = SCREEN_H-box.h;

    fclose(log);

    if( remaining_time>0) remaining_time+=59; // add 59 seconds

    while(1){
        int t1,t2;
        if(remaining_time<0) remaining_time = 0;
        t1 = remaining_time/3600;
        t2 = remaining_time%3600/60;
#ifdef INCLUDE_SECONDS
        sprintf(buf, "%d:%02d:%02d", t1, t2, remaining_time%60);
#else
        sprintf(buf, "%d:%02d", t1, t2);
#endif

        if(ttf_font){
            text = TTF_RenderUTF8_Blended(ttf_font, buf, (SDL_Color){0xc0,0xc0,0xc0});
            SDL_Rect r2 = box;
            r2.x += PADDING;
            r2.y += PADDING;
            SDL_FillRect(screen, &box, 0);
            SDL_FillRect(screen, &box, 0x80000000);
            SDL_BlitSurface(text, NULL, screen, &r2);
            SDL_FreeSurface(text);
        }

        SDL_Flip(screen);

        SDL_Delay(1000);
        remaining_time--;

        if(remaining_time<0) remaining_time = 0;
        if(remaining_time % COUNTER_WRITE_PERIOD == 0 ){
            write_counter(current_day_idx, remaining_time);
        }
        if(remaining_time<=0){
            poweroff();
        }
    }
}

