/**
* ��򵥵Ļ���FFmpeg����Ƶ������
* Simplest FFmpeg Decoder
*
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
*
* ������ʵ������Ƶ�ļ�����ΪYUV���ݡ���ʹ����libavcodec��
* libavformat������򵥵�FFmpeg��Ƶ���뷽��Ľ̡̳�
* ͨ��ѧϰ�����ӿ����˽�FFmpeg�Ľ������̡�
* This software is a simplest decoder based on FFmpeg.
* It decodes video to YUV pixel data.
* It uses libavcodec and libavformat.
* Suitable for beginner of FFmpeg.
*
*/

//�������VS��ֱ�����У���ô���������
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
		printf("����ҷһ����Ƶ�ļ����������ͼ����\n");
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

	//ע�����б������ͽ�����
	av_register_all();
	//avformat_network_init();
	//ΪpFormatCtx�����ڴ�
	pFormatCtx = avformat_alloc_context();

	//����Ƶ�ļ�
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	//�����ļ����ý����
	if (avformat_find_stream_info(pFormatCtx, NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}

	//�ҵ�ý�����е���Ƶ��
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


	//���û���ҵ���Ƶ���򷵻ش���
	if (VideoIndex == -1){
		printf("Didn't find a video stream.û���ҵ���Ƶ��\n");
		return -1;
	}
	if (AudioIndex == -1){
		printf("Didn't find a audio stream.û���ҵ���Ƶ��\n");
		return -1;
	}

	//�����Ƶ���ı���������
	pCodecCtx_Video = pFormatCtx->streams[VideoIndex]->codec;
	//�����Ƶ���ı���������
	pCodecCtx_Audio = pFormatCtx->streams[AudioIndex]->codec;
	//��ffmpeg�в��ҷ��ϴ���Ƶ���ı��������Ķ�Ӧ����Ƶ�����������ش˽�������ָ��
	pCodec_Video = avcodec_find_decoder(pCodecCtx_Video->codec_id);
	//��ffmpeg�в��ҷ��ϴ���Ƶ���ı��������Ķ�Ӧ����Ƶ�����������ش˽�������ָ��
	pCodec_Audio = avcodec_find_decoder(pCodecCtx_Audio->codec_id);
	if (pCodec_Video == NULL){
		printf("This VideoCodec not found.\n");
		return -1;
	}
	if (pCodec_Audio == NULL){
		printf("This AudioCodec not found.\n");
		return -1;
	}
	//�򿪴���Ƶ������
	if (avcodec_open2(pCodecCtx_Video, pCodec_Video, NULL)<0){
		printf("Could not open VideoCodec.\n");
		return -1;
	}

	//������ʱ�ڴ�
	int out_buffer_size_Video = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx_Video->width, pCodecCtx_Video->height, 1);
	unsigned char *out_buffer_Video = (unsigned char *)av_malloc(out_buffer_size_Video);


	//��ʼ��pFrameYUV�������
	AVFrame *pFrameYUV = av_frame_alloc();
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer_Video,
		AV_PIX_FMT_YUV420P, pCodecCtx_Video->width, pCodecCtx_Video->height, 1);

	//Ϊ�����sws_scale()���̵�
	struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx_Video->width, pCodecCtx_Video->height, pCodecCtx_Video->pix_fmt,
		pCodecCtx_Video->width, pCodecCtx_Video->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


	//�򿪴���Ƶ������
	if (avcodec_open2(pCodecCtx_Audio, pCodec_Audio, NULL)<0){
		printf("Could not open AudioCodec.\n");
		return -1;
	}

	//����Ƶ����ر�����ʼ��
	//Out Audio Param  
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	//AAC:1024  MP3:1152  
	int out_nb_samples = pCodecCtx_Audio->frame_size;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate = 44100;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	//Out Buffer Size  
	int out_buffer_size_Audio = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	//Ϊout_buffer_Video��out_buffer_Audio�����ڴ�
	unsigned char *out_buffer_Audio = (unsigned char *)av_malloc(out_buffer_size_Audio);

	AVFrame *pFramePCM = av_frame_alloc();
	//��ʼ��pFramePCM�������
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

	//����Ƶ����ر�����ʼ��
	//����һ���ڴ��AVFrame�������洢��AVPacket��������õ�����
	AVFrame	*pFrame = av_frame_alloc();

	//Ϊpacket�����ڴ棬ʵ�������������һ֡δ����ǰ����Ƶ���ݴ�С
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//av_init_packet(packet);
	
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	//�����Ƶ�ļ���ĸ��ֱ�����Ϣ
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");

	//��packet��λ��С�ķ�ʽ����ȡ��Ƶ�ļ���������ݣ�ͨ��packet->stream_index���ж϶�ȡ������Ƶ��������Ƶ����
	int index = 0;
	while (av_read_frame(pFormatCtx, packet) >= 0){
		//������Ƶ��
		if (packet->stream_index == VideoIndex){//�ж����֡���������Ƿ�����Ƶ����Ӧ������
			ret = avcodec_decode_video2(pCodecCtx_Video, pFrame, &got_picture, packet);//����������ݽ��뵽pFrame��
			if (ret < 0){
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture){//ͨ�����ж����Ƿ��ȡ���ļ�ĩβ��
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx_Video->height,
					pFrameYUV->data, pFrameYUV->linesize);

				y_size = pCodecCtx_Video->width*pCodecCtx_Video->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				printf("Succeed to decode 1 frame!The frame's size=[%d,%d]\n", pCodecCtx_Video->width, pCodecCtx_Video->height);
			}
		}

		//������Ƶ��
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
		av_packet_unref(packet);//�ɰ汾ʹ��av_free_packet(packet);
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
	printf("�˳����������ʱ��Ϊ��%d\n", GetTickCount() - StartTime);
	system("pause");
#endif
	return 0;
}
