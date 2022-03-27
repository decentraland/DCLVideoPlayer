// Example program:
// Using SDL2 to create an application window
#include "player.h"
#include "SDL.h"
#include <stdio.h>
#include "utils.h"
#include "logger.h"

int video_frames = 0;
int video_width = 0;
int video_height = 0;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
MediaPlayerContext* vpc;

void create_texture(int width, int height)
{
  texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);

  void *pixels;
  int pitch;
  if (SDL_LockTexture(texture, NULL, &pixels, &pitch) < 0)
    return;
  memset(pixels, 0, pitch * height);
  SDL_UnlockTexture(texture);
}

void kill()
{
  player_destroy(vpc);
  player_stop_all_threads();

  SDL_DestroyTexture( texture );
  SDL_DestroyRenderer( renderer );
  SDL_DestroyWindow( window );
  texture = NULL;
  window = NULL;
  renderer = NULL;
  SDL_Quit();
}

void update_texture() {
    void *release_ptr = NULL;
    do {
      uint8_t* videoData[3];
      player_grab_video_frame(vpc, &release_ptr, videoData);
      if (release_ptr != NULL) {
        // update texture
        void *pixels;
        int pitch;
        if (SDL_LockTexture(texture, NULL, &pixels, &pitch) < 0)
          return;
        memcpy(pixels, videoData[0], pitch * video_height);
        SDL_UnlockTexture(texture);

        player_release_frame(vpc, release_ptr);
        ++video_frames;
        logging("frames: %d", video_frames);
      } else {
        break;
      }
    } while (1);

    uint8_t *audio_data = NULL;
    int frame_size = 0;
    do {
      audio_data = NULL;
      player_grab_audio_frame(vpc, &release_ptr, &audio_data, &frame_size);
      if (audio_data != NULL) {
        player_release_frame(vpc, release_ptr);
      }
    } while (audio_data != NULL);
}

uint8_t loop() {

  static const unsigned char *keys;
  keys = SDL_GetKeyboardState(NULL);

  SDL_Event e;
  SDL_Rect dest;

  // Clear the window to white
  SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 );
  SDL_RenderClear( renderer );

  // Event loop
  while ( SDL_PollEvent( &e ) != 0 ) {
    switch ( e.type ) {
      case SDL_QUIT:
        return 1;
    }
  }

  update_texture();

  // Render texture
  dest.x = 0;
  dest.y = 0;
  dest.w = video_width;
  dest.h = video_height;
  SDL_RenderCopy(renderer, texture, NULL, &dest);

  // Update window
  SDL_RenderPresent( renderer );

  return 0;
}

uint8_t init_sdl2()
{
  SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

  // Create an application window with the following settings:
  window = SDL_CreateWindow(
          "An SDL2 window",                // window title
          SDL_WINDOWPOS_UNDEFINED,           // initial x position
          SDL_WINDOWPOS_UNDEFINED,           // initial y position
          video_width,                       // width, in pixels
          video_height,                      // height, in pixels
          SDL_WINDOW_OPENGL               // flags - see below
  );

  // Check that the window was successfully created
  if (window == NULL) {
    // In the case that the window could not be made...
    printf("Could not create window: %s\n", SDL_GetError());
    return 1;
  }

  renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
  if ( !renderer ) {
    printf("Error creating renderer: %s\n", SDL_GetError());
    return 1;
  }

  create_texture(video_width, video_height);

  return 0;
}

uint8_t init_player() {

  const char* url = "https://player.vimeo.com/external/691415562.m3u8?s=65096902279bbd8bb19bf9e2b9391c4c7e510402";
  vpc = player_create(url, 1);

  while (player_get_state(vpc) == StateLoading) {
    SDL_Delay(1);
  }

  printf("player_get_state=%d\n", player_get_state(vpc));

  if (player_get_state(vpc) != StateReady) {
    return -1;
  }

  player_get_video_format(vpc, &video_width, &video_height);

  player_set_loop(vpc, 1);
  player_play(vpc);

  return 0;
}

int main(int argc, char* argv[]) {
  if (init_player()) {
    printf("Error init vpc");
    return 1;
  }

  if (init_sdl2()) {
    printf("Error init SDL2");
    return 1;
  }

  printf("Viewer created");

  while ( loop() == 0 ) {
    // wait before processing the next frame
    SDL_Delay(10);
  }

  kill();
  return 0;
}