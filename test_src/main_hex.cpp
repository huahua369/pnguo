#include <SDL3/SDL.h>
#include "SDL3_ttf/SDL_ttf.h"
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define FONT_SIZE 16
#define BYTES_PER_LINE 16

typedef struct {
    unsigned char* data;
    size_t size;
} FileData;

FileData load_file(const char* filename) {
    FileData file_data = { NULL, 0 };
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return file_data;
    }

    fseek(file, 0, SEEK_END);
    file_data.size = ftell(file);
    fseek(file, 0, SEEK_SET);

    file_data.data = (unsigned char*)malloc(file_data.size);
    if (!file_data.data) {
        perror("Failed to allocate memory");
        fclose(file);
        return file_data;
    }

    fread(file_data.data, 1, file_data.size, file);
    fclose(file);
    return file_data;
}

void render_hex_editor(SDL_Renderer* renderer, TTF_Font* font, FileData* file_data) {
    char line[128];
    SDL_Color text_color = { 255, 255, 255, 255 };
    int x = 10, y = 10;

    for (size_t i = 0; i < file_data->size; i += BYTES_PER_LINE) {
        snprintf(line, sizeof(line), "%08zx: ", i);
        for (int j = 0; j < BYTES_PER_LINE; j++) {
            if (i + j < file_data->size) {
                snprintf(line + strlen(line), sizeof(line) - strlen(line), "%02x ", file_data->data[i + j]);
            }
            else {
                snprintf(line + strlen(line), sizeof(line) - strlen(line), "   ");
            }
        }
        snprintf(line + strlen(line), sizeof(line) - strlen(line), " ");
        for (int j = 0; j < BYTES_PER_LINE; j++) {
            if (i + j < file_data->size) {
                char c = file_data->data[i + j];
                snprintf(line + strlen(line), sizeof(line) - strlen(line), "%c", (c >= 32 && c <= 126) ? c : '.');
            }
            else {
                snprintf(line + strlen(line), sizeof(line) - strlen(line), " ");
            }
        }

        SDL_Surface* text_surface = TTF_RenderText_Solid(font, line, text_color);
        SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        SDL_Rect dest_rect = { x, y, text_surface->w, text_surface->h };
        SDL_RenderTexture(renderer, text_texture, NULL, &dest_rect);

        SDL_DestroyTexture(text_texture);
        SDL_DestroySurface(text_surface);

        y += FONT_SIZE;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FileData file_data = load_file(argv[1]);
    if (!file_data.data) {
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Hex Editor", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("font.ttf", FONT_SIZE);
    if (!font) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render_hex_editor(renderer, font, &file_data);

        SDL_RenderPresent(renderer);
    }

    free(file_data.data);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}