///> Include FFMpeg and SDL
// C ����� ��������� ���Խ�ų���� �̷��� ����ϴ°� ����.
extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>

}

//myo include
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>

// The only file that needs to be included to use the Myo C++ SDK is myo.hpp.
#include <myo/myo.hpp>



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
#define BUFFER_SIZE 1024

// compatibility with newer API
// ������ �ٲ�鼭 �޶��� ���� �̷��� Ŀ���ϴ� �� �ϴ�.
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

// Myo �����͸� �޾Ƽ� �������ִ� Ŭ����
class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		: onArm(false), isUnlocked(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose()
	{
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		roll_w = 0;
		pitch_w = 0;
		yaw_w = 0;
		onArm = false;
		isUnlocked = false;
	}

	// onOrientationData() is called whenever the Myo device provides its current orientation, which is represented
	// as a unit quaternion.
	void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat)
	{
		using std::atan2;
		using std::asin;
		using std::sqrt;
		using std::max;
		using std::min;

		// Calculate Euler angles (roll, pitch, and yaw) from the unit quaternion.
		float roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
			1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
		float pitch = asin(max(-1.0f, min(1.0f, 2.0f * (quat.w() * quat.y() - quat.z() * quat.x()))));
		float yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
			1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));

		// Convert the floating point angles in radians to a scale from 0 to 18.
		roll_w = static_cast<int>((roll + (float)M_PI) / (M_PI * 2.0f) * 18);
		pitch_w = static_cast<int>((pitch + (float)M_PI / 2.0f) / M_PI * 18);
		yaw_w = static_cast<int>((yaw + (float)M_PI) / (M_PI * 2.0f) * 18);
	}

	// onPose() is called whenever the Myo detects that the person wearing it has changed their pose, for example,
	// making a fist, or not making a fist anymore.
	void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
	{
		currentPose = pose;

		if (pose != myo::Pose::unknown && pose != myo::Pose::rest) {
			// Tell the Myo to stay unlocked until told otherwise. We do that here so you can hold the poses without the
			// Myo becoming locked.
			myo->unlock(myo::Myo::unlockHold);

			// Notify the Myo that the pose has resulted in an action, in this case changing
			// the text on the screen. The Myo will vibrate.
			myo->notifyUserAction();
		}
		else {
			// Tell the Myo to stay unlocked only for a short period. This allows the Myo to stay unlocked while poses
			// are being performed, but lock after inactivity.
			myo->unlock(myo::Myo::unlockTimed);
		}
	}

	// onArmSync() is called whenever Myo has recognized a Sync Gesture after someone has put it on their
	// arm. This lets Myo know which arm it's on and which way it's facing.
	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection, float rotation,
		myo::WarmupState warmupState)
	{
		onArm = true;
		whichArm = arm;
	}

	// onArmUnsync() is called whenever Myo has detected that it was moved from a stable position on a person's arm after
	// it recognized the arm. Typically this happens when someone takes Myo off of their arm, but it can also happen
	// when Myo is moved around on the arm.
	void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
	{
		onArm = false;
	}

	// onUnlock() is called whenever Myo has become unlocked, and will start delivering pose events.
	void onUnlock(myo::Myo* myo, uint64_t timestamp)
	{
		isUnlocked = true;
	}

	// onLock() is called whenever Myo has become locked. No pose events will be sent until the Myo is unlocked again.
	void onLock(myo::Myo* myo, uint64_t timestamp)
	{
		isUnlocked = false;
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	void print()
	{
		// Clear the current line
		std::cout << '\r';

		// Print out the orientation. Orientation data is always available, even if no arm is currently recognized.
		std::cout << '[' << roll_w << ']'
			<< '[' << pitch_w << ']'
			<< '[' << yaw_w << ']';

		if (onArm) {
			// Print out the lock state, the currently recognized pose, and which arm Myo is being worn on.

			// Pose::toString() provides the human-readable name of a pose. We can also output a Pose directly to an
			// output stream (e.g. std::cout << currentPose;). In this case we want to get the pose name's length so
			// that we can fill the rest of the field with spaces below, so we obtain it as a string using toString().
			std::string poseString = currentPose.toString();

			std::cout << '[' << (isUnlocked ? "unlocked" : "locked  ") << ']'
				<< '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
				<< '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
		}
		else {
			// Print out a placeholder for the arm and pose when Myo doesn't currently know which arm it's on.
			std::cout << '[' << std::string(8, ' ') << ']' << "[?]" << '[' << std::string(14, ' ') << ']';
		}

		std::cout << std::flush;
	}

	void sendright(){

	}

	// These values are set by onArmSync() and onArmUnsync() above.
	bool onArm;
	myo::Arm whichArm;

	// This is set by onUnlocked() and onLocked() above.
	bool isUnlocked;

	// These values are set by onOrientationData() and onPose() above.
	int roll_w, pitch_w, yaw_w;
	myo::Pose currentPose;
};


// ���� �������� �̹����� �����ϴ� �Լ�
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf_s(szFilename, "frame%d.ppm", iFrame);
	fopen_s(&pFile, szFilename, "wb");
	if (pFile == NULL)
		return;

	// Write header
	// ȭ�� ������� Į���� ����(?) �� ���� 0~255 ���� ����
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y<height; y++)
		fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}

int main(int argc, char *argv[]) {
	// Initalizing these to NULL prevents segfaults!
	AVFormatContext   *pFormatCtx = NULL;
	int               i, videoStream;
	AVCodecContext    *pCodecCtxOrig = NULL;
	AVCodecContext    *pCodecCtx = NULL; // �ڵ� ��Ʈ�ѷ�(?) �̰� ���� ����.
	AVCodec           *pCodec = NULL; // ������ ���ڵ��� �ڵ�
	AVFrame           *pFrame = NULL; // �������� ��� �����.
	AVFrame           *pFrameRGB = NULL; // RGB�� ���� ���������� ������� ���´�.
	AVPacket          packet;
	int               frameFinished;
	int               numBytes;
	uint8_t           *buffer = NULL;
	struct SwsContext *sws_ctx = NULL; // RGB���� ������ ����ϴ��� �ڼ��� �𸣰���

	//SDL ���� ����
	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;

	//���� �� �ƿ��� ���� ����
	int rect_w = 0;
	int rect_h = 0;

	// ����

	WSADATA   wsaData;

	SOCKET   ClientSocket;
	SOCKADDR_IN  ToServer;   
	SOCKADDR_IN  FromServer;

	int   FromServer_Size;
	int   Recv_Size;   int   Send_Size;

	char   Buffer[BUFFER_SIZE] = { "Message~" };
	char   left_m[BUFFER_SIZE] = { "����" };
	char   right_m[BUFFER_SIZE] = { "������" };
	USHORT   ServerPort = 3333;
	
	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try {
	// ������� ���̿� �ʱ�ȭ
	// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
	// publishing your application. The Hub provides access to one or more Myos.
	myo::Hub hub("com.example.hello-myo");

	// ���̿� ã���� ...
	std::cout << "Attempting to find a Myo..." << std::endl;

	// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
	// immediately.
	// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
	// if that fails, the function will return a null pointer.
	myo::Myo* myo = hub.waitForMyo(10000);

	// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
	if (!myo) {
		throw std::runtime_error("Unable to find a Myo!");
	}

	// We've found a Myo.
	std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

	// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
	DataCollector collector;

	// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
	// Hub::run() to send events to all registered device listeners.
	hub.addListener(&collector);

	//---������� ���̿� �ʱ�ȭ

	// ���� �ʱ�ȭ

	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
	{
		printf("WinSock �ʱ�ȭ�κп��� ���� �߻�.n");
		WSACleanup();
		exit(0);
	}

	memset(&ToServer, 0, sizeof(ToServer));
	memset(&FromServer, 0, sizeof(FromServer));

	ToServer.sin_family = AF_INET;
	// �ܺξ����Ƿ� ��Ʈ�� �ϰ� �;��µ� ���� �������� �ܺξ����Ƿ� ��Ʈ�������� �ϸ� �����Ͱ� ������ �ȵȴ�.
	// �ƹ����� ��Ʈ�� �����ִ� ��Ŀ� ������ �ִ°� ������ �ذ��� ���� ������.
	ToServer.sin_addr.s_addr = inet_addr("192.168.0.15"); 
	ToServer.sin_port = htons(ServerPort); // ��Ʈ��ȣ

	ClientSocket = socket(AF_INET, SOCK_DGRAM, 0);// udp 

	if (ClientSocket == INVALID_SOCKET)
	{
		printf("������ �����Ҽ� �����ϴ�.");
		closesocket(ClientSocket);
		WSACleanup();
		exit(0);
	}
	
	// Register all formats and codecs
	av_register_all();
	//network init �̰� ���� ������ ������ ���� �ϴ� ������. (�� ������ �� �𸣰ڽ�)
	avformat_network_init();

	//SDL �ʱ�ȭ 
	//���� SDL������ ������ ����
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	// Open video file
	// ���� �Ǵ� ������ ��Ʈ���� ����.
	if (avformat_open_input(&pFormatCtx, "tcp://14.35.11.234:2222", NULL, NULL) != 0)
	{
		return -1; // Couldn't open file
	}
	
	// Retrieve stream information
	// ������ ��Ʈ���� ������ ���´�.
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		return -1; // Couldn't find stream information
	}
	// Dump information about file onto standard error
	// ������ �߻��� ��� ���� ������ �����ϴ� �ڵ�� ����
	av_dump_format(pFormatCtx, 0, "tcp://14.35.11.234:2222", 0);

	// Find the first video stream
	// ���� ��Ʈ���� ã�°��� - � ������ ������ ��Ʈ������ �Ǻ� ( �츮�� h.264�� �����Ǿ�������...)
	videoStream = -1;
	for (i = 0; (unsigned)i<pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	// Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Copy context
	// �� ���� �𸣰����� �׳� ���� �ʰ� �����ؼ� ����Ѵ�.
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
		return -1; // Could not open codec

	// Allocate video frame
	pFrame = av_frame_alloc();




	// Allocate an AVFrame structure

	//... �ⲯ RGB ��ȯ ������ �����ߴµ� ��½ÿ� ���� �ʴ´�.
	/*pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
		return -1;*/

	// Determine required buffer size and allocate buffer
	// ������ ����� �����´�
	numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,pCodecCtx->height);
	buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture

	// RGB��ȯ�� ���� �ʴ´�.
	/*avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_YUV420P,
		pCodecCtx->width, pCodecCtx->height);*/

	// Make a screen to put our video
	// ��ũ���� ����
#ifndef __DARWIN__
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
	if (!screen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	// �̹����� ��ũ���� �׸�
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
		pCodecCtx->height,
		SDL_YV12_OVERLAY,
		screen);

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width,
		pCodecCtx->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
		);

	// Read frames and save first five frames to disk
	i = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// ���̿� ����

		// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
		// In this case, we wish to update our display 20 times a second, so we run for 1000/20 milliseconds.
		hub.run(1000 / 500);
		// After processing events, we call the print() member function we defined above to print out the values we've
		// obtained from any events that have occurred.
		collector.print();
		
		// ���̿� ���� �������


		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			// ���� �������� ��Ʈ�� �̹����� ��ȯ
			if (frameFinished) {
				SDL_LockYUVOverlay(bmp);

				AVPicture pict;
				pict.data[0] = bmp->pixels[0];
				pict.data[1] = bmp->pixels[2];
				pict.data[2] = bmp->pixels[1];

				pict.linesize[0] = bmp->pitches[0];
				pict.linesize[1] = bmp->pitches[2];
				pict.linesize[2] = bmp->pitches[1];

				
				// Convert the image into YUV format that SDL uses
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pict.data, pict.linesize);

				SDL_UnlockYUVOverlay(bmp);

				// ����Ʈ��������� ���� �ܾƿ��� �ϱ����� ������������ ����� ����
				rect.x = -rect_w/2;
				rect.y = -rect_h/2;
				rect.w = pCodecCtx->width + rect_w;
				rect.h = pCodecCtx->height + rect_h;
				SDL_DisplayYUVOverlay(bmp, &rect);

			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		SDL_PollEvent(&event);

		// ���̿��� ������ üũ�ؼ� �޽��� �۽�
		if (collector.currentPose == myo::Pose::waveOut){
			Send_Size = sendto(ClientSocket, right_m, sizeof(right_m), 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
		}	
		if (collector.currentPose == myo::Pose::waveIn){
			Send_Size = sendto(ClientSocket, left_m, sizeof(left_m), 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
		}

		// Ű �̺�Ʈ�� �޴� �Լ�
		switch (event.type) {
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
			break;
		case SDL_KEYDOWN:
			/* Check the SDLKey values and move change the coords */
			switch (event.key.keysym.sym){
			case SDLK_LEFT:
				// ���ڿ� �۽�
				Send_Size = sendto(ClientSocket, left_m, sizeof(left_m), 0,(struct sockaddr*) &ToServer, sizeof(ToServer));
				break;
			case SDLK_RIGHT:
				// ���ڿ� �۽�
				Send_Size = sendto(ClientSocket, right_m, sizeof(right_m), 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
				break;
			case SDLK_UP: // �� ��
				if (rect_w < 300)
				{
					rect_h += 20;
					rect_w += 30;
				}
				break;
			case SDLK_DOWN: // �� �ƿ�
				if (0 < rect_w)
				{
					rect_h -= 20;
					rect_w -= 30;
				}				
				break;
			case SDLK_x: // �ñ׷� ����
				SDL_Quit();
				exit(0);
				break;
			case SDLK_s:
				
			default:
				break;
			}
		default:
			break;
		}

	}



	// Free the RGB image
	av_free(buffer);
	av_frame_free(&pFrameRGB);

	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	// ���� �ݱ�
	closesocket(ClientSocket); 
	WSACleanup();

	return 0;

	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
	
}

