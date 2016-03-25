/**
* ��򵥵Ļ���FFmpeg����Ƶ������2(SDL������)
* Simplest FFmpeg Player 2(SDL Update)
*
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* ��2��ʹ��SDL2.0ȡ���˵�һ���е�SDL1.2
* Version 2 use SDL 2.0 instead of SDL 1.2 in version 1.
*
* ������ʵ������Ƶ�ļ��Ľ������ʾ(֧��HEVC��H.264��MPEG2��)��
* ����򵥵�FFmpeg��Ƶ���뷽��Ľ̡̳�
* ͨ��ѧϰ�����ӿ����˽�FFmpeg�Ľ������̡�
* ���汾��ʹ��SDL��Ϣ����ˢ����Ƶ���档
* This software is a simplest video player based on FFmpeg.
* Suitable for beginner of FFmpeg.
*
* ��ע:
* ��׼���ڲ�����Ƶ��ʱ�򣬻�����ʾʹ����ʱ40ms�ķ�ʽ����ô�������������
* ��1��SDL�����Ĵ����޷��ƶ���һֱ��ʾ��æµ״̬
* ��2��������ʾ�������ϸ��40msһ֡����Ϊ��û�п��ǽ����ʱ�䡣
* SU��SDL Update��������Ƶ����Ĺ����У�����ʹ����ʱ40ms�ķ�ʽ�����Ǵ�����
* һ���̣߳�ÿ��40ms����һ���Զ������Ϣ����֪���������н�����ʾ��������֮��
* ��1��SDL�����Ĵ��ڿ����ƶ���
* ��2��������ʾ���ϸ��40msһ֡
* Remark:
* Standard Version use's SDL_Delay() to control video's frame rate, it has 2
* disadvantages:
* (1)SDL's Screen can't be moved and always "Busy".
* (2)Frame rate can't be accurate because it doesn't consider the time consumed
* by avcodec_decode_video2()
* SU��SDL Update��Version solved 2 problems above. It create a thread to send SDL
* Event every 40ms to tell the main loop to decode and show video frames.
*/

//�������VS��ֱ�����У���ô���������
#define ISINVS
#define SHOW_TIME

//�����˺꽫ʹ��SDL2.x API������ʹ��SDL1.x API
#define  SDLVERSION2X

extern "C"
{
#include "libavcodec/avcodec.h"		
#include "libavformat/avformat.h"	
#include "libswscale/swscale.h"		
#include "libavutil/imgutils.h"		
};

#pragma comment(lib,"../ffmpeg/lib/avcodec.lib")
#pragma comment(lib,"../ffmpeg/lib/avdevice.lib")
#pragma comment(lib,"../ffmpeg/lib/avfilter.lib")
#pragma comment(lib,"../ffmpeg/lib/avformat.lib")
#pragma comment(lib,"../ffmpeg/lib/avutil.lib")
#pragma comment(lib,"../ffmpeg/lib/postproc.lib")
#pragma comment(lib,"../ffmpeg/lib/swresample.lib")
#pragma comment(lib,"../ffmpeg/lib/swscale.lib")

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

//#define OUTPUT_YUV420P

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit = 0;
int thread_pause = 0;



bool fopenEx(FILE* &_File, const char *filename, const char* _Mode)
{
	bool IsSucess = false;
	//FILE *fp;
#if _MSC_VER<1400   //VS 2005�汾����                    
	if ((_File = fopen(filename, _Mode)) != NULL)
#else  //VS 2005�汾����
	errno_t Rerr = fopen_s(&_File, filename, _Mode);
	if (Rerr == 0)
#endif 
	{
		IsSucess = true;
	}
	return IsSucess;
}


//ˢ���߳�
int sfp_refresh_thread(void *opaque){
	thread_exit = 0;
	thread_pause = 0;

	while (!thread_exit) {
		if (!thread_pause){
			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		SDL_Delay(40);
	}
	thread_exit = 0;
	thread_pause = 0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}


int main(int argc, char* argv[])
{
	//�������������ʱ��
#ifdef SHOW_TIME
	DWORD StartTime = GetTickCount();
#endif
	AVFormatContext	*pFormatCtx;
	unsigned int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;

	//------------SDL----------------
	int screen_w, screen_h;
	int ret, got_picture;

#ifdef ISINVS
	char* filepath = "output.yuv";
#else
	char* filepath = "../bin/output.yuv";
#endif
#ifdef OUTPUT_YUV420P   
	FILE *fp_yuv;
	fopenEx(fp_yuv, filepath, "wb+");
#endif    

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
#ifdef ISINVS
	if (avformat_open_input(&pFormatCtx, "../bin/Mp4Video.mp4", NULL, NULL) != 0){
#else
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0){
#endif
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
	if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
		videoindex = i;
		break;
	}
	if (videoindex == -1){
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL){
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	//���°汾ffmpeg�ﱻ������
	//out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//�°汾�Ƽ����������
	out_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));


	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	//Output Info-----------------------------
	printf("---------------- File Information ---------------\n");
	av_dump_format(pFormatCtx, 0, argv[1], 0);
	printf("-------------------------------------------------\n");

	struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	//SDL 2.0 Support for multiple windows
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
#ifdef SDLVERSION2X
	SDL_Window *screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL);
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
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

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	//����һ���߳�
#ifdef SDLVERSION2X
	SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
#else
	SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL);
#endif
	//------------SDL End------------
	//Event Loop
	SDL_Event event;
	for (;;) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT){
			//------------------------------
			if (av_read_frame(pFormatCtx, packet) >= 0){
				if (packet->stream_index == videoindex){
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
					if (ret < 0){
						printf("Decode Error.\n");
						return -1;
					}
					if (got_picture){
						sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
						
#ifdef SDLVERSION2X
						//SDL---------------------------
						SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);
						SDL_RenderClear(sdlRenderer);
						//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
						SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
						SDL_RenderPresent(sdlRenderer);
						//SDL End-----------------------
#else
						SDL_LockYUVOverlay(bmp);
						//��bmp��ؼ��ĳ�Ա�������ݸ�pFrameYUV������sws_scale�����ʼ��
						pFrameYUV->data[0] = bmp->pixels[0];
						pFrameYUV->data[1] = bmp->pixels[2];
						pFrameYUV->data[2] = bmp->pixels[1];
						pFrameYUV->linesize[0] = bmp->pitches[0];
						pFrameYUV->linesize[1] = bmp->pitches[2];
						pFrameYUV->linesize[2] = bmp->pitches[1];

						sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
							pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

						SDL_UnlockYUVOverlay(bmp);

						SDL_DisplayYUVOverlay(bmp, &sdlRect);
#endif
						//File Option Start-----------------------
						int y_size = pCodecCtx->width*pCodecCtx->height;
#ifdef OUTPUT_YUV420P   
						fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
						fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
						fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
#endif
						printf("Succeed to decode 1 frame!\n");

						//File Option End-----------------------

					}
				}
				av_packet_unref(packet);
				//av_free_packet(packet);
			}
			else{
				//Exit Thread
				thread_exit = 1;
			}
		}
		else if (event.type == SDL_KEYDOWN){
			//Pause
			if (event.key.keysym.sym == SDLK_SPACE)
				thread_pause = !thread_pause;
		}
		else if (event.type == SDL_QUIT){
			thread_exit = 1;
		}
		else if (event.type == SFM_BREAK_EVENT){
			break;
		}

	}

	//��ϴ��������ʣ�µ���Ƶ֡
	//Flush Option Start-----------------------
	//flush decoder��ע�ͣ���ϴ��������
	//FIX: Flush Frames remained in Codec
	while (1) {
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (ret < 0)
			break;
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize);

		int y_size = pCodecCtx->width*pCodecCtx->height;

#ifdef OUTPUT_YUV420P  
		fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
		fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
#endif

		printf("Flush Decoder: Succeed to decode 1 frame!\n");
	}
	//Flush Option End-----------------------


	sws_freeContext(img_convert_ctx);

#ifdef OUTPUT_YUV420P  
	fclose(fp_yuv);
#endif

	SDL_Quit();
	//--------------
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

#ifdef SHOW_TIME
	printf("�˳����������ʱ��Ϊ��%d\n", GetTickCount() - StartTime);
	system("pause");
#endif
	return 0;
}