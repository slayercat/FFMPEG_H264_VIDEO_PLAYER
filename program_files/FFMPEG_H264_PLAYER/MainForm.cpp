#include "stdafx.h"
#include "MainForm.h"

namespace FFMPEG_H264_PLAYER {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;

	//全局，用于处理码流信息
	//修改函数：StartPlayFile用于启动
	AVFormatContext* pFormatCtx;

	//全局，用于处理解码器信息
	//修改函数：StartPlayFile用于启动
	AVCodecContext* pCodecCtx;

	//全局，用于标志第一个视频流
	//修改函数：StartPlayFile用于启动
	int videoStream;

	//全局，用于确认当前帧已经结束，非0为已经结束
	//修改函数：threadRunDecodeAndPlay用于转换
	int frameFinished;

	//每绘制一帧的暂停时间
	unsigned int waitTime;

	System::Void MainForm::drawFrame(Image^ imageToDraw)
	{
		this->pictureBox1->Image = imageToDraw;
	}

	//保存BMP文件的函数  
	System::Drawing::Bitmap^ GetBMPImage (AVFrame *pFrameRGB, int width, int height, int bpp)   
	{   

		BITMAPFILEHEADER bmpheader;
		BITMAPINFO bmpinfo;


		bmpheader.bfType = ('M'<<8)|'B';
		bmpheader.bfReserved1 = 0;
		bmpheader.bfReserved2 = 0;
		bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		//文件大小
		bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp/8;
		/*----注意，这里的bmpinfo.bmiHeader.biHeight变量的正负决定bmp文件的存储方式，如果
		为负值，表示像素是倒过来的*/
		bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpinfo.bmiHeader.biWidth = width;
		bmpinfo.bmiHeader.biHeight = -height;
		bmpinfo.bmiHeader.biPlanes = 1;
		bmpinfo.bmiHeader.biBitCount = 24;
		bmpinfo.bmiHeader.biCompression = BI_RGB;
		bmpinfo.bmiHeader.biSizeImage = 0;
		bmpinfo.bmiHeader.biXPelsPerMeter = 100;
		bmpinfo.bmiHeader.biYPelsPerMeter = 100;
		bmpinfo.bmiHeader.biClrUsed = 0;
		bmpinfo.bmiHeader.biClrImportant = 0;

		
		//用于保存输出值
		array<Byte>^ out_buf = gcnew array<Byte>(bmpheader.bfSize);
		
		unsigned int currectPoint = 0;
		Marshal::Copy(IntPtr(&bmpheader), out_buf, currectPoint, Int32(sizeof(BITMAPFILEHEADER))); 
		currectPoint += sizeof(BITMAPFILEHEADER);
		Marshal::Copy(IntPtr(&bmpinfo.bmiHeader), out_buf, currectPoint, sizeof(BITMAPINFOHEADER)); 
		currectPoint += sizeof(BITMAPINFOHEADER);
		for(int y=0; y<height; y++)
		{
			Marshal::Copy(IntPtr(pFrameRGB->data[0]+y*pFrameRGB->linesize[0]), out_buf, currectPoint,width*3);
			currectPoint += width*3;
		}


		return gcnew System::Drawing::Bitmap(gcnew System::IO::MemoryStream(out_buf));
	}   


	//解码并播放，使用pFormatCtx
	System::Void MainForm::threadRunDecodeAndPlay()
	{
		b_IsThreadPlayRunning = true;

		AVFrame* pFrame = avcodec_alloc_frame();
		AVFrame* pFrameRGB = avcodec_alloc_frame();
		if(pFrameRGB==NULL)
			throw gcnew Exception("分配帧错误");

		//分配缓存
		uint8_t* buffer = (uint8_t *)
			av_malloc(
			avpicture_get_size(PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height)
			* sizeof(uint8_t)
			);
		avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

		AVPacket packet;
		//转换环境
		SwsContext* img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
			pCodecCtx->width, pCodecCtx->height, PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

		if(img_convert_ctx == NULL)
		{
			throw gcnew Exception("无法初始化图像转换环境");
		}

		
		//结构至此全部完毕，开始解码播放。
		//线程主循环，b_IsThreadPlayRunning为false时会停止。
		while(b_IsThreadPlayRunning)
		{
			if(!(av_read_frame(pFormatCtx, &packet)>=0))
			{
				break;
			}
			if(packet.stream_index==videoStream)
			{
				avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			}
			if(frameFinished)
			{
				//转换为rgb格式
				sws_scale(img_convert_ctx,pFrame->data, pFrame->linesize, 0, pCodecCtx->height,pFrameRGB->data, pFrameRGB->linesize);
				//当下可以显示了
				//CALLBACK
				
				this->BeginInvoke(RunDrawFrame,
					GetBMPImage(pFrameRGB, pCodecCtx->width, pCodecCtx->height, 24));
				//根据帧率暂停
				System::Threading::Thread::Sleep(waitTime);
				
			}
		}
		
		//完成清理工作
		av_free_packet(&packet);
		av_free(buffer);
		av_free(pFrameRGB);
		av_free(pFrame);
		avcodec_close(pCodecCtx);
		av_close_input_file(pFormatCtx);
		b_IsThreadPlayRunning = false;
	}

	//该函数用于打开文件、寻找解码器并解码
	System::Void MainForm::StartPlayFile(System::String^ filename)
	{
		pFormatCtx = 0;
		IntPtr ip = Marshal::StringToHGlobalAnsi(filename);
		const char* pTemp = static_cast<const char*>(ip.ToPointer());

		if(avformat_open_input(&pFormatCtx, pTemp, 0, 0)!=0)
			throw gcnew Exception("无法打开文件");

		if(av_find_stream_info(pFormatCtx)<0)
			throw gcnew Exception("文件中未找到流信息");

		av_dump_format(pFormatCtx, 0, pTemp, 0);


		//用于找到容器文件中第一个视频流
		videoStream = -1;
		for(unsigned int i=0; i<pFormatCtx->nb_streams; i++)
		{
			if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) 
			{
				videoStream = i;
				
				waitTime = 1000.0 /
						(
							((double)pFormatCtx -> streams[i]->r_frame_rate.num) /
								(pFormatCtx -> streams[i]->r_frame_rate.den));

				System::Console::WriteLine(waitTime.ToString());

				break;
			}
		}
		if( videoStream == -1)
		{
			throw gcnew Exception("未找到视频流");
		}

		//确定码流信息
		pCodecCtx=pFormatCtx->streams[videoStream]->codec;

		//找解码器
		AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL) {
			throw gcnew Exception("不支持的视频流");
		}

		if(avcodec_open(pCodecCtx, pCodec)<0)
			throw gcnew Exception("解码错误");

		//开启新线程用于解码
		if(threadPlay != nullptr)
		{
			StopPlayThread();
		}

		threadPlay = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(this, &MainForm::threadRunDecodeAndPlay));
		threadPlay -> Start();
	}


	System::Void MainForm::StopPlayThread()
	{
		int stopWaitCount = 0;
		
		const int maxStopWaitCount = 10;
		const unsigned int delayTime = 500;

		b_IsThreadPlayRunning = false;
		while(threadPlay->IsAlive)
		{
			if(stopWaitCount < maxStopWaitCount)
			{
				++stopWaitCount;
				System::Threading::Thread::Sleep(delayTime);
			}
			else
			{
				break;
			}
		}

		if(threadPlay->IsAlive)
		{
			threadPlay->Abort();
		}
	}

}