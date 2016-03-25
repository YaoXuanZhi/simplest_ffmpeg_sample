
/**
* ��򵥵�SDL2������Ƶ�����ӣ�SDL2����RGB/YUV��
* Simplest Video Play SDL2 (SDL2 play RGB/YUV)
*
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* ������ʹ��SDL2����RGB/YUV��Ƶ�������ݡ�SDLʵ�����ǶԵײ��ͼ
* API��Direct3D��OpenGL���ķ�װ��ʹ���������Լ���ֱ�ӵ��õײ�
* API��
*
* �������ò�������:
*
* [��ʼ��]
* SDL_Init(): ��ʼ��SDL��
* SDL_CreateWindow(): �������ڣ�Window����
* SDL_CreateRenderer(): ���ڴ��ڴ�����Ⱦ����Render����
* SDL_CreateTexture(): ��������Texture����
*
* [ѭ����Ⱦ����]
* SDL_UpdateTexture(): ������������ݡ�
* SDL_RenderCopy(): �����Ƹ���Ⱦ����
* SDL_RenderPresent(): ��ʾ��
*
* This software plays RGB/YUV raw video data using SDL2.
* SDL is a wrapper of low-level API (Direct3D, OpenGL).
* Use SDL is much easier than directly call these low-level API.
*
* The process is shown as follows:
*
* [Init]
* SDL_Init(): Init SDL.
* SDL_CreateWindow(): Create a Window.
* SDL_CreateRenderer(): Create a Render.
* SDL_CreateTexture(): Create a Texture.
*
* [Loop to Render data]
* SDL_UpdateTexture(): Set Texture's data.
* SDL_RenderCopy(): Copy Texture to Render.
* SDL_RenderPresent(): Show.
*/

//�������VS��ֱ�����У���ô���������
#define ISINVS
//�鿴�������е���ʱ��
#define SHOW_TIME
//����ѭ������
#define LOOPPLAY

//�����˺꽫ʹ��SDL2.x API������ʹ��SDL1.x API
//#define  SDLVERSION2X

#ifdef SDLVERSION2X
extern "C"
{
#include "../SDL2.X/include/SDL.h"
};
#pragma comment(lib,"../SDL2.X/lib/x86/SDL2.lib")
#pragma comment(lib,"../SDL2.X/lib/x86/SDL2main.lib")
#else
extern "C"
{
#include "../SDL1.X/include/SDL.h"
};
#pragma comment(lib,"../SDL1.X/lib/x86/SDL.lib")
#pragma comment(lib,"../SDL1.X/lib/x86/SDLmain.lib")
#endif




#include <stdio.h>
#include <Windows.h>

const int bpp = 12;
int screen_w = 960, screen_h = 540;
//int screen_w = 1280, screen_h = 720;
//int screen_w = 500, screen_h = 500;
//const int pixel_w = 320, pixel_h = 180;
//const int pixel_w = 1280, pixel_h = 720;
//���ֱ�Ӷ�ȡYUV���ݣ���ô��Ҫ������Ƶ�ļ�ԭ���ߴ����ֶ��趨
//�����ֱ�Ӷ�ȡ��Ƶ�������ڴ����ݣ���ôͨ�������������Ŀ��Ի�ȡ
const int pixel_w = 960, pixel_h = 540;

//Refresh Event  
#define REFRESH_EVENT  (SDL_USEREVENT + 1)  

#define BREAK_EVENT  (SDL_USEREVENT + 2)  

int thread_exit = 0;
int thread_pause = 0;

//ˢ���߳�
int refresh_video(void *opaque){
	thread_exit = 0;
	thread_pause = 0;

	while (!thread_exit) {
		if (!thread_pause){
			SDL_Event event;
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		//ʵ��25֡/����ٶȲ���
		SDL_Delay(40);
	}
	thread_exit = 0;
	thread_pause = 0;
	//Break
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}


//����VC6.0��VSϵ�е�fopen����
bool fopenEx(FILE* &_File, const char *filename, const char* _Mode)
{
	bool IsSucess = false;
#if _MSC_VER<1400   //VS 2005�汾����                    
	if ((_File = fopen(filename, _Mode)) != NULL)
#else  //VS 2005�汾����
	errno_t Rerr = fopen_s(&_File, filename, _Mode);
	if (Rerr == 0)
#endif 
		IsSucess = true;
	return IsSucess;
}



int main(int argc, char* argv[])
{
	//�������������ʱ��
#ifdef SHOW_TIME
	DWORD StartTime = GetTickCount();
#endif

	char filepath[256];
#ifndef ISINVS
	if (argc < 2)
	{
		printf("����ҷһ��.yuv�ļ����������ͼ����\n");
		system("pause");
		exit(1);
	}
	else
		sprintf_s(filepath, "%s", argv[1]);
#else
	sprintf_s(filepath, "../bin/output.yuv");
#endif

	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

#ifdef SDLVERSION2X
	//SDL 2.0 Support for multiple windows  
	SDL_Window *screen = SDL_CreateWindow("Simplest Video Play SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	//��SDL���������Ѵ��ڵĴ���
	//SDL_CreateWindowFrom();
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_SetWindowTitle(screen,"Simplest FFmpeg Player(SDL2.x)");
	Uint32 pixformat = 0;

	//IYUV: Y + U + V  (3 planes)  
	//YV12: Y + V + U  (3 planes)  
	pixformat = SDL_PIXELFORMAT_IYUV;

	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);
#else
	SDL_Surface *screen = SDL_SetVideoMode(screen_w, screen_h, 0, SDL_RESIZABLE);
	SDL_Overlay *bmp = SDL_CreateYUVOverlay(screen_w, screen_h, SDL_YV12_OVERLAY, screen);
	SDL_WM_SetCaption("Simplest FFmpeg Player(SDL1.x)", NULL);
#endif
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}

	SDL_Rect sdlRect;
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	FILE *fp;
	fopenEx(fp, filepath, "rb+");

	if (fp == NULL){
		printf("cannot open this file\n");
		return -1;
	}
#ifdef SDLVERSION2X
	unsigned char *buffer;
	buffer = (unsigned char*)malloc(pixel_w*pixel_h*bpp / 8);
	SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);//SDL2.x
#else
	unsigned char* pY;
	unsigned char* pU;
	unsigned char* pV;

	pY = (unsigned char*)malloc(pixel_w*pixel_h);
	pU = (unsigned char*)malloc(pixel_w*pixel_h / 4);
	pV = (unsigned char*)malloc(pixel_w*pixel_h / 4);
	SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL);
#endif

	SDL_Event event;
	int FrameIndex = 0;
	while (1){
		//Wait  
		SDL_WaitEvent(&event);
		if (event.type == REFRESH_EVENT){
			//������Ƶ���Ŵ���
			sdlRect.x = 0;
			sdlRect.y = 0;
			sdlRect.w = screen_w;
			sdlRect.h = screen_h;

#ifdef SDLVERSION2X
			SDL_UpdateTexture(sdlTexture, NULL, buffer, pixel_w);
			if (fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp) != pixel_w*pixel_h*bpp / 8){
#ifdef LOOPPLAY
				// Loop  
				fseek(fp, 0, SEEK_SET);
				fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp);
#else
				//��ȡ���Զ��˳�
				thread_exit = 1;
#endif
			}
			//SDL2.x
			SDL_RenderClear(sdlRenderer);
			SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
			SDL_RenderPresent(sdlRenderer);
#else
			//SDL_LockSurface(screen);
			SDL_LockYUVOverlay(bmp);
			//���ļ��ж�ȡһ������Ϣ���ڴ��У��ֱ�YUV���ݴ����pY��pU��pV��
			fread(pY, 1, pixel_w*pixel_h, fp);
			fread(pU, 1, pixel_w*pixel_h / 4, fp);

			if (fread(pV, 1, pixel_w*pixel_h / 4, fp) != pixel_w*pixel_h / 4)//�����ȡ����YUV���ݺ��˳�
			{
#ifdef LOOPPLAY
				// Loop  
				fseek(fp, 0, SEEK_SET);
				fread(pY, 1, pixel_w*pixel_h, fp);
				fread(pU, 1, pixel_w*pixel_h / 4, fp);
				fread(pV, 1, pixel_w*pixel_h / 4, fp);
#else
				//��ȡ���Զ��˳�
				thread_exit = 1;
#endif
			}
			//���ڴ��е����ݿ�����bmp->pixels��
			memcpy(bmp->pixels[0], pY, pixel_w*pixel_h);
			memcpy(bmp->pixels[1], pV, pixel_w*pixel_h / 4);
			memcpy(bmp->pixels[2], pU, pixel_w*pixel_h / 4);

			SDL_UnlockYUVOverlay(bmp);
			//SDL_UnlockSurface(screen);
			SDL_DisplayYUVOverlay(bmp, &sdlRect);
#endif
			printf("Succeed to decode %d frame!\n", FrameIndex);
			FrameIndex++;
		}
#ifdef SDLVERSION2X
		else if (event.type == SDL_WINDOWEVENT)
#else
		else if (event.type == SDL_VIDEORESIZE)
#endif
		{
			printf("���ô��ڴ�С\n");
#ifdef SDLVERSION2X
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
#else
			//SDL1.xû��SDL_GetWindowSize����������ʹ��event.resize����ȡ
			screen_w = event.resize.w;
			screen_h = event.resize.h;
#endif
		}
		else if (event.type == SDL_KEYDOWN){
			//Pause
			if (event.key.keysym.sym == SDLK_SPACE)
				thread_pause = !thread_pause;
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			printf("���������\n");
		}
		else if (event.type == SDL_QUIT){
			thread_exit = 1;
		}
		else if (event.type == BREAK_EVENT){
			break;
		}
	}
	SDL_Quit();

#ifdef SHOW_TIME
	printf("�˳����������ʱ��Ϊ��%d\n", GetTickCount() - StartTime);
	system("pause");
#endif
	return 0;
}