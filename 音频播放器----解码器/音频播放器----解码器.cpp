/**
* ��򵥵Ļ���FFmpeg����Ƶ������
* Simplest FFmpeg Audio Decoder
*
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* ��������Խ���Ƶ������mp3��AAC�ȣ�����ΪPCM�������ݡ�
* ����򵥵�FFmpeg��Ƶ���뷽��Ľ̡̳�
* ͨ��ѧϰ�����ӿ����˽�FFmpeg�Ľ������̡�
*
* This software decode audio streams (AAC,MP3 ...) to PCM data.
* Suitable for beginner of FFmpeg.
*
*/

#include <stdlib.h>  
#include <string.h>  
extern "C"
{
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
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

 
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio  
  
int main(int argc, char* argv[])  
{  
    AVFormatContext *pFormatCtx;  
    size_t             i, audioStream;  
    AVCodecContext  *pCodecCtx;  
    AVCodec         *pCodec;  
    AVPacket        *packet;  
    uint8_t         *out_buffer;  
    AVFrame         *pFrame;  
    int ret;  
    uint32_t len = 0;  
    int got_picture;  
    int index = 0;  
    int64_t in_channel_layout;  
    struct SwrContext *au_convert_ctx;  
  
	FILE *pFile;
	fopenEx(pFile,"../bin/Audio.pcm", "wb");
    char url[]="../bin/skycity1.mp3";  
  
    av_register_all();  
    avformat_network_init();  
    pFormatCtx = avformat_alloc_context();  

    //Open  
    if(avformat_open_input(&pFormatCtx,url,NULL,NULL)!=0){  
        printf("Couldn't open input stream.\n");  
        return -1;  
    }  
    // Retrieve stream information  
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){  
        printf("Couldn't find stream information.\n");  
        return -1;  
    }  
    // Dump valid information onto standard error  
    av_dump_format(pFormatCtx, 0, url, false);  
  
    // Find the first audio stream  
    audioStream=-1;  
    for(i=0; i < pFormatCtx->nb_streams; i++)  
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){  
            audioStream=i;  
            break;  
        }  
  
    if(audioStream==-1){  
        printf("Didn't find a audio stream.\n");  
        return -1;  
    }  
  
    // Get a pointer to the codec context for the audio stream  
    pCodecCtx=pFormatCtx->streams[audioStream]->codec;  
  
    // Find the decoder for the audio stream  
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);  
    if(pCodec==NULL){  
        printf("Codec not found.\n");  
        return -1;  
    }  
  
    // Open codec  
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){  
        printf("Could not open codec.\n");  
        return -1;  
    }  
  
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));  
    av_init_packet(packet);  
  
    //Out Audio Param  
    uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;  
    //nb_samples: AAC-1024 MP3-1152  
    int out_nb_samples=pCodecCtx->frame_size;  
    AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;  
    int out_sample_rate=44100;  
    int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);  
    //Out Buffer Size  
    int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);  
  
    out_buffer=(uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);  
    pFrame=av_frame_alloc();  
  
    //FIX:Some Codec's Context Information is missing  
    in_channel_layout=av_get_default_channel_layout(pCodecCtx->channels);  
    //Swr  
    au_convert_ctx = swr_alloc();  
    au_convert_ctx=swr_alloc_set_opts(au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,  
        in_channel_layout,pCodecCtx->sample_fmt , pCodecCtx->sample_rate,0, NULL);  
    swr_init(au_convert_ctx);  
  
    while(av_read_frame(pFormatCtx, packet)>=0){  
        if(packet->stream_index==audioStream){  
  
            ret = avcodec_decode_audio4( pCodecCtx, pFrame,&got_picture, packet);  
            if ( ret < 0 ) {  
                printf("Error in decoding audio frame.\n");  
                return -1;  
            }  
            if ( got_picture > 0 ){  
                swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)pFrame->data , pFrame->nb_samples);  
  
                printf("index:%5d\t pts:%lld\t packet size:%d\n",index,packet->pts,packet->size);  
                //Write PCM  
                fwrite(out_buffer, 1, out_buffer_size, pFile);  
                index++;  
            }  
        }  
        //av_free_packet(packet);  
		av_packet_unref(packet);
		
    }  
  
    swr_free(&au_convert_ctx);  
  
    fclose(pFile);  
  
    av_free(out_buffer);  
    // Close the codec  
    avcodec_close(pCodecCtx);  
    // Close the video file  
    avformat_close_input(&pFormatCtx);  
  
    return 0;  
}  