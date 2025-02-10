#include <SDL3/SDL.h>
#include "SDL3_ttf/SDL_ttf.h"
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <vector>
#include <string>
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define FONT_SIZE 16
#define BYTES_PER_LINE 16
/*
二进制文件编辑器
1.打开保存文件、备份文件
2.滚动偏移、解析出16进制字符串、同步修改、显示整数字符串编码等
3.渲染解码部分信息
*/
typedef struct {
	unsigned char* data;
	size_t size;
	TTF_TextEngine* engine;
	std::vector<TTF_Text*>* dt;
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
	SDL_Color text_color = { 158, 158, 255, 255 };
	int x = 10, y = 10;
	SDL_Rect rect = {};
	SDL_GetRenderViewport(renderer, &rect);
	int nc = rect.h / FONT_SIZE;
	int dnc = file_data->size / BYTES_PER_LINE;
	dnc += file_data->size % BYTES_PER_LINE > 0 ? 1 : 0;
	int newnc = std::min(nc + 1, dnc);
	if (!file_data->dt)
	{ 
		file_data->dt = new std::vector<TTF_Text*>();
	}
	if (file_data->dt->size() != newnc)
	{
		for (auto it : *(file_data->dt)) {
			TTF_DestroyText(it);
		}
		file_data->dt->clear();
	}
	if (file_data->dt->empty())
	{
		auto data = file_data->data;
		for (size_t i = 0; i < file_data->size && newnc > 0; i += BYTES_PER_LINE, newnc--) {
			snprintf(line, sizeof(line), "%08zx: ", i);
			for (int j = 0; j < BYTES_PER_LINE; j++) {
				if (i + j < file_data->size) {
					snprintf(line + strlen(line), sizeof(line) - strlen(line), "%02x ", data[i + j]);
				}
				else {
					snprintf(line + strlen(line), sizeof(line) - strlen(line), "   ");
				}
			}
			snprintf(line + strlen(line), sizeof(line) - strlen(line), " ");
			for (int j = 0; j < BYTES_PER_LINE; j++) {
				if (i + j < file_data->size) {
					char c = data[i + j];
					snprintf(line + strlen(line), sizeof(line) - strlen(line), "%c", (c >= 32 && c <= 126) ? c : '.');
				}
				else {
					snprintf(line + strlen(line), sizeof(line) - strlen(line), " ");
				}
			}
			auto dt = TTF_CreateText(file_data->engine, font, line, strlen(line));
			TTF_SetTextColor(dt, text_color.r, text_color.g, text_color.b, text_color.a);
			if (dt)
				file_data->dt->push_back(dt);
		}
	}
	for (auto dt : *file_data->dt)
	{
		TTF_DrawRendererText(dt, x, y);
		y += FONT_SIZE;
	}
}
int sdlm(const char* fn)
{

	FileData file_data = load_file(fn);
	if (!file_data.data) {
		return 1;
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	if (TTF_Init() < 0) {
		printf("TTF could not initialize! TTF_Error: %s\n", SDL_GetError());
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

	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
	if (!renderer) {
		printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	TTF_Font* font = TTF_OpenFont("C:\\WINDOWS\\FONTS\\CONSOLA.TTF", FONT_SIZE);
	if (!font) {
		printf("Failed to load font! TTF_Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		TTF_Quit();
		SDL_Quit();
		return 1;
	}
	file_data.engine = TTF_CreateRendererTextEngine(renderer);
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
		SDL_Delay(10);
	}

	free(file_data.data);
	TTF_CloseFont(font);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
	return 0;
}
int main(int argc, char* argv[]) {
	system("rd /s /q E:\\temcpp\\SymbolCache\\hex.pdb");
	const char* fn = argv[1];
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		fn = "dxcompiler.dll";
		//fn = "b2";
	}
	return sdlm(fn);
}