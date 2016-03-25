
//如果是在VS里直接运行，那么开启这个宏
#define ISINVS

//开启循环播放
#define LOOPPLAY

//开启此宏将使用SDL2.x API，否则使用SDL1.x API
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

int Video_Thead(FILE* fp_yuv);
int Audio_Thead(FILE* fp_pcm);

const int bpp = 12;
int screen_w = 960, screen_h = 540;
//int screen_w = 1280, screen_h = 720;
//int screen_w = 500, screen_h = 500;
//const int pixel_w = 320, pixel_h = 180;
//const int pixel_w = 1280, pixel_h = 720;
//如果直接读取YUV数据，那么需要根据视频文件原来尺寸来手动设定
//如果是直接读取视频解码后的内存数据，那么通过编码器上下文可以获取
const int pixel_w = 960, pixel_h = 540;

//Refresh Event  
#define REFRESH_EVENT  (SDL_USEREVENT + 1)  

#define BREAK_EVENT  (SDL_USEREVENT + 2)  

int thread_exit = 0;
int thread_pause = 0;
//Buffer:  
//|-----------|-------------|  
//chunk-------pos---len-----|  
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

//刷新视频线程线程
int refresh_video(void *opaque){
	thread_exit = 0;
	thread_pause = 0;

	while (!thread_exit) {
		if (!thread_pause){
			SDL_Event event;
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		//实现25帧/秒的速度播放
		SDL_Delay(33);
	}
	thread_exit = 0;
	thread_pause = 0;
	//Break
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

//刷新音频线程线程
int refresh_audio(void *opaque){
	thread_exit = 0;
	thread_pause = 0;

	while (!thread_exit) {
		if (!thread_pause){
			SDL_Event event;
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		//实现25帧/秒的速度播放
		SDL_Delay(1);
	}
	thread_exit = 0;
	thread_pause = 0;
	//Break
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}


/* Audio Callback
* The audio function callback takes the following parameters:
* stream: A pointer to the audio buffer to be filled
* len: The length (in bytes) of the audio buffer
*
*/
void  fill_audio(void *udata, Uint8 *stream, int len){
#ifdef SDLVERSION2X
	SDL_memset(stream, 0, len);
#endif
	if (audio_len == 0)        /*  Only  play  if  we  have  data  left  */
		return;
	len = (len>audio_len ? audio_len : len);   /*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

//兼容VC6.0和VS系列的fopen函数
bool fopenEx(FILE* &_File, const char *filename, const char* _Mode)
{
	bool IsSucess = false;
#if _MSC_VER<1400   //VS 2005版本以下                    
	if ((_File = fopen(filename, _Mode)) != NULL)
#else  //VS 2005版本以上
	errno_t Rerr = fopen_s(&_File, filename, _Mode);
	if (Rerr == 0)
#endif 
		IsSucess = true;
	return IsSucess;
}

int PlayAudio(void *opaque);
int PlayVideo(void *opaque);

int main(int argc, char* argv[])
{
	char filepath_yuv[256], filepath_pcm[256];
#ifndef ISINVS
	if (argc < 2)
	{
		printf("请拖曳一个.yuv文件到本程序的图标上\n");
		system("pause");
		exit(1);
	}
	else
		sprintf_s(filepath, "%s", argv[1]);
#else
	sprintf_s(filepath_yuv, "../bin/Video.yuv");
	sprintf_s(filepath_pcm, "../bin/Audio.pcm");
#endif

	FILE *fp_yuv, *fp_pcm;
	fopenEx(fp_yuv, filepath_yuv, "rb+");
	fopenEx(fp_pcm, filepath_pcm, "rb+");

	if (fp_yuv == NULL){
		printf("cannot open this YUVfile\n");
		return -1;
	}
	if (fp_pcm == NULL){
		printf("cannot open this PCMfile\n");
		return -1;
	}

#ifdef SDLVERSION2X
	SDL_Thread *refresh_thread_video = SDL_CreateThread(PlayVideo, NULL, fp_yuv);//SDL2.x
	SDL_Thread *refresh_thread_audio = SDL_CreateThread(PlayAudio, NULL, fp_pcm);//SDL2.x
#else
	SDL_Thread *refresh_thread_video = SDL_CreateThread(PlayVideo,  fp_yuv);//SDL1.x
	SDL_Thread *refresh_thread_audio = SDL_CreateThread(PlayAudio,  fp_pcm);//SDL1.x
#endif

	while (!thread_exit)
	{

	}

	fclose(fp_yuv);
	fclose(fp_pcm);
	return 0;
}



int PlayAudio(void *opaque)
{
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	FILE *fp_pcm = (FILE *)opaque;
	//SDL_AudioSpec  
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = 44100;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = 2;
	wanted_spec.silence = 0;
	wanted_spec.samples = 1024;
	wanted_spec.callback = fill_audio;

	if (SDL_OpenAudio(&wanted_spec, NULL)<0){
		printf("can't open audio.\n");
		return -1;
	}

	int pcm_buffer_size = 4096;
	char *pcm_buffer = (char *)malloc(pcm_buffer_size);
	int data_count = 0;

	//Play  
	SDL_PauseAudio(0);

	int FrameIndex = 0;

	while (1){
		if (fread(pcm_buffer, 1, pcm_buffer_size, fp_pcm) != pcm_buffer_size){
#ifdef LOOPPLAY
			// Loop  
			fseek(fp_pcm, 0, SEEK_SET);
			fread(pcm_buffer, 1, pcm_buffer_size, fp_pcm);
			data_count = 0;
#else
			//SDL_Quit();
			exit(1);
#endif
		}
		printf("Now Playing %10d Bytes data.\n", data_count);

		data_count += pcm_buffer_size;
		//Set audio buffer (PCM data)  
		audio_chunk = (Uint8 *)pcm_buffer;
		//Audio buffer length  
		audio_len = pcm_buffer_size;
		audio_pos = audio_chunk;

		while (audio_len>0)//Wait until finish  
			SDL_Delay(1);
	}
	SDL_Quit();
	return 0;
}

int PlayVideo(void *opaque)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	FILE *fp_yuv = (FILE *)opaque;
#ifdef SDLVERSION2X
	//SDL 2.0 Support for multiple windows  
	SDL_Window *screen = SDL_CreateWindow("Simplest Video Play SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	//让SDL窗口依赖已存在的窗口
	//SDL_CreateWindowFrom();
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_SetWindowTitle(screen, "Simplest FFmpeg Player(SDL2.x)");
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

#ifdef SDLVERSION2X
	unsigned char *buffer;
	buffer = (unsigned char*)malloc(pixel_w*pixel_h*bpp / 8);
	SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL,NULL);
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
			//重置视频播放窗口
			sdlRect.x = 0;
			sdlRect.y = 0;
			sdlRect.w = screen_w;
			sdlRect.h = screen_h;

#ifdef SDLVERSION2X
			SDL_UpdateTexture(sdlTexture, NULL, buffer, pixel_w);
			if (fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp_yuv) != pixel_w*pixel_h*bpp / 8){
#ifdef LOOPPLAY
				// Loop  
				fseek(fp_yuv, 0, SEEK_SET);
				fread(buffer, 1, pixel_w*pixel_h*bpp / 8, fp_yuv);
#else
				//读取完自动退出
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
			//从文件中读取一部分信息到内存中，分别将YUV数据存放在pY、pU、pV中
			fread(pY, 1, pixel_w*pixel_h, fp_yuv);
			fread(pU, 1, pixel_w*pixel_h / 4, fp_yuv);

			if (fread(pV, 1, pixel_w*pixel_h / 4, fp_yuv) != pixel_w*pixel_h / 4)//假设读取所有YUV数据后，退出
			{
#ifdef LOOPPLAY
				// Loop  
				fseek(fp_yuv, 0, SEEK_SET);
				fread(pY, 1, pixel_w*pixel_h, fp_yuv);
				fread(pU, 1, pixel_w*pixel_h / 4, fp_yuv);
				fread(pV, 1, pixel_w*pixel_h / 4, fp_yuv);
#else
				//读取完自动退出
				thread_exit = 1;
#endif
			}
			//将内存中的数据拷贝到bmp->pixels中
			memcpy(bmp->pixels[0], pY, pixel_w*pixel_h);
			memcpy(bmp->pixels[1], pV, pixel_w*pixel_h / 4);
			memcpy(bmp->pixels[2], pU, pixel_w*pixel_h / 4);

			SDL_UnlockYUVOverlay(bmp);
			//SDL_UnlockSurface(screen);
			SDL_DisplayYUVOverlay(bmp, &sdlRect);
#endif
			printf("Succeed to show %d frame yuv data!\n", FrameIndex);
			FrameIndex++;
		}
#ifdef SDLVERSION2X
		else if (event.type == SDL_WINDOWEVENT)
#else
		else if (event.type == SDL_VIDEORESIZE)
#endif
		{
			printf("重置窗口大小\n");
#ifdef SDLVERSION2X
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
#else
			//SDL1.x没有SDL_GetWindowSize函数，所以使用event.resize来获取
			screen_w = event.resize.w;
			screen_h = event.resize.h;
#endif
		}
		else if (event.type == SDL_KEYDOWN){
			//Pause
			if (event.key.keysym.sym == SDLK_SPACE)
				thread_pause = !thread_pause;
			thread_pause == 1 ? SDL_PauseAudio(1):SDL_PauseAudio(0);
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			printf("点击窗口了\n");
		}
		else if (event.type == SDL_QUIT){
			thread_exit = 1;
		}
		else if (event.type == BREAK_EVENT){
			break;
		}
	}
	SDL_Quit();
	return 0;
}