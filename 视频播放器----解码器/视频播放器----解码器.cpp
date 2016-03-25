/**
* 最简单的基于FFmpeg的视频解码器
* Simplest FFmpeg Decoder
*
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
*
* 本程序实现了视频文件解码为YUV数据。它使用了libavcodec和
* libavformat。是最简单的FFmpeg视频解码方面的教程。
* 通过学习本例子可以了解FFmpeg的解码流程。
* This software is a simplest decoder based on FFmpeg.
* It decodes video to YUV pixel data.
* It uses libavcodec and libavformat.
* Suitable for beginner of FFmpeg.
*
*/

//如果是在VS里直接运行，那么开启这个宏
#define ISINVS
#define SHOW_TIME


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

#include <stdlib.h>  
#include <string.h>  
#include <Windows.h>

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


int main(int argc, char* argv[])
{
	//计算程序运行总时间
#ifdef SHOW_TIME
	DWORD StartTime = GetTickCount();
#endif
	AVFormatContext	*pFormatCtx;
	size_t				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame, *pFrameYUV;
	unsigned char *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	char filepath[256];
#ifndef ISINVS
	if (argc < 2)
	{
		printf("请拖曳一个视频文件到本程序的图标上\n");
		system("pause");
		exit(1);
	}
	else
		sprintf_s(filepath, "%s", argv[1]);
#else
	sprintf_s(filepath, "../bin/Mp4Video.mp4");
#endif
	FILE *fp_yuv;
#ifndef ISINVS
	fopenEx(fp_yuv,"output.yuv", "wb+");
#else
	fopenEx(fp_yuv, "../bin/output.yuv", "wb+");
#endif

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0){
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
	out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);



	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	while (av_read_frame(pFormatCtx, packet) >= 0){
		if (packet->stream_index == videoindex){
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0){
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture){
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);

				y_size = pCodecCtx->width*pCodecCtx->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				printf("Succeed to decode 1 frame!The frame's size=[%d,%d]\n", pCodecCtx->width, pCodecCtx->height);

			}
		}
		av_packet_unref(packet);//旧版本使用av_free_packet(packet);
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (1) {
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (ret < 0)
			break;
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize);

		int y_size = pCodecCtx->width*pCodecCtx->height;
		fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
		fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

		printf("Flush Decoder: Succeed to decode 1 frame!\n");
	}

	sws_freeContext(img_convert_ctx);

	fclose(fp_yuv);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

#ifdef SHOW_TIME
	printf("此程序的运行总时长为：%d\n", GetTickCount() - StartTime);
	system("pause");
#endif
	return 0;
}
