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

	//ȫ�֣����ڴ���������Ϣ
	//�޸ĺ�����StartPlayFile��������
	AVFormatContext* pFormatCtx;

	//ȫ�֣����ڴ����������Ϣ
	//�޸ĺ�����StartPlayFile��������
	AVCodecContext* pCodecCtx;

	//ȫ�֣����ڱ�־��һ����Ƶ��
	//�޸ĺ�����StartPlayFile��������
	int videoStream;

	//ȫ�֣�����ȷ�ϵ�ǰ֡�Ѿ���������0Ϊ�Ѿ�����
	//�޸ĺ�����threadRunDecodeAndPlay����ת��
	int frameFinished;

	//ÿ����һ֡����ͣʱ��
	unsigned int waitTime;

	System::Void MainForm::drawFrame(Image^ imageToDraw)
	{
		this->pictureBox1->Image = imageToDraw;
	}

	//����BMP�ļ��ĺ���  
	System::Drawing::Bitmap^ GetBMPImage (AVFrame *pFrameRGB, int width, int height, int bpp)   
	{   

		BITMAPFILEHEADER bmpheader;
		BITMAPINFO bmpinfo;


		bmpheader.bfType = ('M'<<8)|'B';
		bmpheader.bfReserved1 = 0;
		bmpheader.bfReserved2 = 0;
		bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		//�ļ���С
		bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp/8;
		/*----ע�⣬�����bmpinfo.bmiHeader.biHeight��������������bmp�ļ��Ĵ洢��ʽ�����
		Ϊ��ֵ����ʾ�����ǵ�������*/
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

		
		//���ڱ������ֵ
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


	//���벢���ţ�ʹ��pFormatCtx
	System::Void MainForm::threadRunDecodeAndPlay()
	{
		b_IsThreadPlayRunning = true;

		AVFrame* pFrame = avcodec_alloc_frame();
		AVFrame* pFrameRGB = avcodec_alloc_frame();
		if(pFrameRGB==NULL)
			throw gcnew Exception("����֡����");

		//���仺��
		uint8_t* buffer = (uint8_t *)
			av_malloc(
			avpicture_get_size(PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height)
			* sizeof(uint8_t)
			);
		avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

		AVPacket packet;
		//ת������
		SwsContext* img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
			pCodecCtx->width, pCodecCtx->height, PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

		if(img_convert_ctx == NULL)
		{
			throw gcnew Exception("�޷���ʼ��ͼ��ת������");
		}

		
		//�ṹ����ȫ����ϣ���ʼ���벥�š�
		//�߳���ѭ����b_IsThreadPlayRunningΪfalseʱ��ֹͣ��
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
				//ת��Ϊrgb��ʽ
				sws_scale(img_convert_ctx,pFrame->data, pFrame->linesize, 0, pCodecCtx->height,pFrameRGB->data, pFrameRGB->linesize);
				//���¿�����ʾ��
				//CALLBACK
				
				this->BeginInvoke(RunDrawFrame,
					GetBMPImage(pFrameRGB, pCodecCtx->width, pCodecCtx->height, 24));
				//����֡����ͣ
				System::Threading::Thread::Sleep(waitTime);
				
			}
		}
		
		//���������
		av_free_packet(&packet);
		av_free(buffer);
		av_free(pFrameRGB);
		av_free(pFrame);
		avcodec_close(pCodecCtx);
		av_close_input_file(pFormatCtx);
		b_IsThreadPlayRunning = false;
	}

	//�ú������ڴ��ļ���Ѱ�ҽ�����������
	System::Void MainForm::StartPlayFile(System::String^ filename)
	{
		pFormatCtx = 0;
		IntPtr ip = Marshal::StringToHGlobalAnsi(filename);
		const char* pTemp = static_cast<const char*>(ip.ToPointer());

		if(avformat_open_input(&pFormatCtx, pTemp, 0, 0)!=0)
			throw gcnew Exception("�޷����ļ�");

		if(av_find_stream_info(pFormatCtx)<0)
			throw gcnew Exception("�ļ���δ�ҵ�����Ϣ");

		av_dump_format(pFormatCtx, 0, pTemp, 0);


		//�����ҵ������ļ��е�һ����Ƶ��
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
			throw gcnew Exception("δ�ҵ���Ƶ��");
		}

		//ȷ��������Ϣ
		pCodecCtx=pFormatCtx->streams[videoStream]->codec;

		//�ҽ�����
		AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL) {
			throw gcnew Exception("��֧�ֵ���Ƶ��");
		}

		if(avcodec_open(pCodecCtx, pCodec)<0)
			throw gcnew Exception("�������");

		//�������߳����ڽ���
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