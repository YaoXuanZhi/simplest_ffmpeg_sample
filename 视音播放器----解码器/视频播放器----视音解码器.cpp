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
#include "libswresample/swresample.h" 
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
	size_t				i, VideoIndex, AudioIndex;
	AVCodecContext	*pCodecCtx_Video, *pCodecCtx_Audio;;
	AVCodec			*pCodec_Video, *pCodec_Audio; 
	
	
	
	int y_size;
	int ret, got_picture;
	
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
	FILE *fp_pcm;
#ifndef ISINVS
	fopenEx(fp_yuv,"output.yuv", "wb+");
#else
	fopenEx(fp_yuv, "../bin/Video.yuv", "wb+");
	fopenEx(fp_pcm, "../bin/Audio.pcm", "wb+");
#endif

	//注册所有编码器和解码器
	av_register_all();
	//avformat_network_init();
	//为pFormatCtx分配内存
	pFormatCtx = avformat_alloc_context();

	//打开视频文件
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	//查找文件里的媒体流
	if (avformat_find_stream_info(pFormatCtx, NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}

	//找到媒体流中的视频流
	VideoIndex = -1;
	AudioIndex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		//if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
		//	VideoIndex = i;
		//	break;
		//}
		switch (pFormatCtx->streams[i]->codec->codec_type)
		{
		case AVMEDIA_TYPE_VIDEO:
			VideoIndex = i;
			break;
		case AVMEDIA_TYPE_AUDIO:
			AudioIndex = i;
			break;
		default:
			break;
		}
	}


	//如果没有找到视频流则返回错误
	if (VideoIndex == -1){
		printf("Didn't find a video stream.没有找到视频轨\n");
		return -1;
	}
	if (AudioIndex == -1){
		printf("Didn't find a audio stream.没有找到音频轨\n");
		return -1;
	}

	//获得视频流的编码上下文
	pCodecCtx_Video = pFormatCtx->streams[VideoIndex]->codec;
	//获得音频流的编码上下文
	pCodecCtx_Audio = pFormatCtx->streams[AudioIndex]->codec;
	//在ffmpeg中查找符合此视频流的编码上下文对应的视频解码器，返回此解码器的指针
	pCodec_Video = avcodec_find_decoder(pCodecCtx_Video->codec_id);
	//在ffmpeg中查找符合此音频流的编码上下文对应的音频解码器，返回此解码器的指针
	pCodec_Audio = avcodec_find_decoder(pCodecCtx_Audio->codec_id);
	if (pCodec_Video == NULL){
		printf("This VideoCodec not found.\n");
		return -1;
	}
	if (pCodec_Audio == NULL){
		printf("This AudioCodec not found.\n");
		return -1;
	}
	//打开此视频解码器
	if (avcodec_open2(pCodecCtx_Video, pCodec_Video, NULL)<0){
		printf("Could not open VideoCodec.\n");
		return -1;
	}

	//分配临时内存
	int out_buffer_size_Video = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx_Video->width, pCodecCtx_Video->height, 1);
	unsigned char *out_buffer_Video = (unsigned char *)av_malloc(out_buffer_size_Video);


	//初始化pFrameYUV里的数据
	AVFrame *pFrameYUV = av_frame_alloc();
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer_Video,
		AV_PIX_FMT_YUV420P, pCodecCtx_Video->width, pCodecCtx_Video->height, 1);

	//为后面的sws_scale()做铺垫
	struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx_Video->width, pCodecCtx_Video->height, pCodecCtx_Video->pix_fmt,
		pCodecCtx_Video->width, pCodecCtx_Video->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


	//打开此音频解码器
	if (avcodec_open2(pCodecCtx_Audio, pCodec_Audio, NULL)<0){
		printf("Could not open AudioCodec.\n");
		return -1;
	}

	//与音频流相关变量初始化
	//Out Audio Param  
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	//AAC:1024  MP3:1152  
	int out_nb_samples = pCodecCtx_Audio->frame_size;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate = 44100;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	//Out Buffer Size  
	int out_buffer_size_Audio = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	//为out_buffer_Video和out_buffer_Audio分配内存
	unsigned char *out_buffer_Audio = (unsigned char *)av_malloc(out_buffer_size_Audio);

	AVFrame *pFramePCM = av_frame_alloc();
	//初始化pFramePCM里的数据
	av_samples_fill_arrays(pFramePCM->data, pFramePCM->linesize, out_buffer_Audio,
		out_channels, pCodecCtx_Audio->frame_size, out_sample_fmt, 1);

	printf("Bitrate:\t %3d\n", pFormatCtx->bit_rate);
	printf("Decoder Name:\t %s\n", pCodecCtx_Audio->codec->long_name);
	printf("Channels:\t %d\n", pCodecCtx_Audio->channels);
	printf("Sample per Second\t %d \n", pCodecCtx_Audio->sample_rate);
	//FIX:Some Codec's Context Information is missing 
	int64_t in_channel_layout = av_get_default_channel_layout(pCodecCtx_Audio->channels);
	//Swr  
	struct SwrContext *au_convert_ctx;
	au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
		in_channel_layout, pCodecCtx_Audio->sample_fmt, pCodecCtx_Audio->sample_rate, 0, NULL);
	swr_init(au_convert_ctx);

	//与视频流相关变量初始化
	//分配一个内存给AVFrame，用来存储从AVPacket解码后所得的数据
	AVFrame	*pFrame = av_frame_alloc();

	//为packet分配内存，实质上是它来存放一帧未解码前的视频数据大小
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//av_init_packet(packet);
	
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	//输出视频文件里的各种编码信息
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");

	//以packet单位大小的方式来读取视频文件里的流数据，通过packet->stream_index来判断读取的是视频流还是音频流等
	int index = 0;
	while (av_read_frame(pFormatCtx, packet) >= 0){
		//处理视频流
		if (packet->stream_index == VideoIndex){//判断这个帧的流索引是否是视频流对应的索引
			ret = avcodec_decode_video2(pCodecCtx_Video, pFrame, &got_picture, packet);//将这个包数据解码到pFrame中
			if (ret < 0){
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture){//通过此判断来是否读取到文件末尾了
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx_Video->height,
					pFrameYUV->data, pFrameYUV->linesize);

				y_size = pCodecCtx_Video->width*pCodecCtx_Video->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				printf("Succeed to decode 1 frame!The frame's size=[%d,%d]\n", pCodecCtx_Video->width, pCodecCtx_Video->height);
			}
		}

		//处理音频流
		if (packet->stream_index == AudioIndex){

			ret = avcodec_decode_audio4(pCodecCtx_Audio, pFrame, &got_picture, packet);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				return -1;
			}
			if (got_picture > 0){
				swr_convert(au_convert_ctx, &out_buffer_Audio, /*MAX_AUDIO_FRAME_SIZE*/192000, (const uint8_t **)pFrame->data, pFrame->nb_samples);

				printf("index:%5d\t pts:%lld\t packet size:%d\n", index, packet->pts, packet->size);
				//Write PCM  
				fwrite(out_buffer_Audio, 1, out_buffer_size_Audio, fp_pcm);
				index++;
			}
		}
		av_packet_unref(packet);//旧版本使用av_free_packet(packet);
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (1) {
		ret = avcodec_decode_video2(pCodecCtx_Video, pFrame, &got_picture, packet);
		if (ret < 0)
			break;
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx_Video->height,
			pFrameYUV->data, pFrameYUV->linesize);

		int y_size = pCodecCtx_Video->width*pCodecCtx_Video->height;
		fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
		fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

		printf("Flush Decoder: Succeed to decode 1 frame!\n");
	}

	sws_freeContext(img_convert_ctx);
	swr_free(&au_convert_ctx);

	fclose(fp_yuv);
	fclose(fp_pcm);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx_Video);
	avformat_close_input(&pFormatCtx);

#ifdef SHOW_TIME
	printf("此程序的运行总时长为：%d\n", GetTickCount() - StartTime);
	system("pause");
#endif
	return 0;
}
