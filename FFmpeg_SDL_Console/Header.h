#pragma once

///> Include FFMpeg and SDL
extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>
}

///> Library Link On Windows System
// �ڲ� ���� �ȵǼ� ffmpeg�� ��� ���̺귯���� �� ����־��
#pragma comment( lib, "avformat.lib" )	
#pragma comment( lib, "avutil.lib" )
#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avdevice.lib" )	
#pragma comment( lib, "avfilter.lib" )
#pragma comment( lib, "postproc.lib" )
#pragma comment( lib, "swresample.lib" )
#pragma comment( lib, "swscale.lib" )
//SDL ���̺귯��
#pragma comment( lib, "SDLmain.lib")
#pragma comment( lib, "SDL.lib")

//�� ����
#pragma comment (lib,"ws2_32.lib")