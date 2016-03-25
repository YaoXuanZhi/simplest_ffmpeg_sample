/**
* ��򵥵�SDL2������Ƶ�����ӣ�SDL2����PCM��
* Simplest Audio Play SDL2 (SDL2 play PCM)
*
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* ������ʹ��SDL2����PCM��Ƶ�������ݡ�SDLʵ�����ǶԵײ��ͼ
* API��Direct3D��OpenGL���ķ�װ��ʹ���������Լ���ֱ�ӵ��õײ�
* API��
*
* �������ò�������:
*
* [��ʼ��]
* SDL_Init(): ��ʼ��SDL��
* SDL_OpenAudio(): ���ݲ������洢��SDL_AudioSpec������Ƶ�豸��
* SDL_PauseAudio(): ������Ƶ���ݡ�
*
* [ѭ����������]
* SDL_Delay(): ��ʱ�ȴ�������ɡ�
*
* This software plays PCM raw audio data using SDL2.
* SDL is a wrapper of low-level API (DirectSound).
* Use SDL is much easier than directly call these low-level API.
*
* The process is shown as follows:
*
* [Init]
* SDL_Init(): Init SDL.
* SDL_OpenAudio(): Opens the audio device with the desired
*                  parameters (In SDL_AudioSpec).
* SDL_PauseAudio(): Play Audio.
*
* [Loop to play data]
* SDL_Delay(): Wait for completetion of playback.
*/


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
#include <tchar.h>  

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

//Buffer:  
//|-----------|-------------|  
//chunk-------pos---len-----|  
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

/* Audio Callback
* The audio function callback takes the following parameters:
* stream: A pointer to the audio buffer to be filled
* len: The length (in bytes) of the audio buffer
*
*/
void  fill_audio(void *udata, Uint8 *stream, int len){
#ifdef SDLVERSION2X
	//SDL 2.0  
	SDL_memset(stream, 0, len);
#endif
	if (audio_len == 0)        /*  Only  play  if  we  have  data  left  */
		return;
	len = (len>audio_len ? audio_len : len);   /*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

int main(int argc, char* argv[])
{
	//Init  
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
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

	FILE *fp;
	fopenEx(fp,"../bin/Audio.pcm", "rb+");
	if (fp == NULL){
		printf("cannot open this file\n");
		return -1;
	}

	int pcm_buffer_size = 4096;
	char *pcm_buffer = (char *)malloc(pcm_buffer_size);
	int data_count = 0;

	//Play  
	SDL_PauseAudio(0);

	while (1){
		if (fread(pcm_buffer, 1, pcm_buffer_size, fp) != pcm_buffer_size){
			// Loop  
			fseek(fp, 0, SEEK_SET);
			fread(pcm_buffer, 1, pcm_buffer_size, fp);
			data_count = 0;
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
	free(pcm_buffer);
	SDL_Quit();
	return 0;
}