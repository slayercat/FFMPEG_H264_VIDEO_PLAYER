// FFMPEG_H264_PLAYER.cpp: 主项目文件。

#include "stdafx.h"

using namespace System;


[System::STAThreadAttribute]
int main(array<System::String ^> ^args)
{
	av_register_all();


	System::Windows::Forms::Application::EnableVisualStyles();
	System::Windows::Forms::Application::SetCompatibleTextRenderingDefault(false);

	System::Windows::Forms::Application::Run(gcnew FFMPEG_H264_PLAYER::MainForm());

    
	return 0;
}
