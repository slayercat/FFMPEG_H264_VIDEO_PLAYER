#pragma once
#include "stdafx.h"

using namespace std;


namespace FFMPEG_H264_PLAYER {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;


	/// <summary>
	/// MainForm 摘要
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			//
			//在此处添加构造函数代码
			//
			RunDrawFrame = gcnew DrawFrameDelegate(this, &MainForm::drawFrame);
			b_IsThreadPlayRunning = false;
		}

	protected:
		/// <summary>
		/// 清理所有正在使用的资源。
		/// </summary>
		~MainForm()
		{
			if(threadPlay != nullptr)
			{
				StopPlayThread();
			}
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  label1;
	protected: 
	private: System::Windows::Forms::TextBox^  textBox1;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;
	private: System::Windows::Forms::PictureBox^  pictureBox1;
	private: System::Windows::Forms::Button^  button2;
	private: System::Windows::Forms::Button^  button3;



	private:
		/// <summary>
		/// 必需的设计器变量。
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// 设计器支持所需的方法 - 不要
		/// 使用代码编辑器修改此方法的内容。
		/// </summary>
		void InitializeComponent(void)
		{
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->button3 = (gcnew System::Windows::Forms::Button());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(12, 9);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(35, 12);
			this->label1->TabIndex = 0;
			this->label1->Text = L"path:";
			// 
			// textBox1
			// 
			this->textBox1->Location = System::Drawing::Point(53, 6);
			this->textBox1->Name = L"textBox1";
			this->textBox1->Size = System::Drawing::Size(551, 21);
			this->textBox1->TabIndex = 1;
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(610, 4);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(62, 23);
			this->button1->TabIndex = 2;
			this->button1->Text = L"open";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &MainForm::button1_Click);
			// 
			// openFileDialog1
			// 
			this->openFileDialog1->FileName = L"openFileDialog1";
			// 
			// pictureBox1
			// 
			this->pictureBox1->Location = System::Drawing::Point(14, 33);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(794, 401);
			this->pictureBox1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::Zoom;
			this->pictureBox1->TabIndex = 3;
			this->pictureBox1->TabStop = false;
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(678, 4);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(62, 23);
			this->button2->TabIndex = 4;
			this->button2->Text = L"play";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &MainForm::button2_Click);
			// 
			// button3
			// 
			this->button3->Location = System::Drawing::Point(746, 4);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(62, 23);
			this->button3->TabIndex = 5;
			this->button3->Text = L"stop";
			this->button3->UseVisualStyleBackColor = true;
			this->button3->Click += gcnew System::EventHandler(this, &MainForm::button3_Click);
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(816, 446);
			this->Controls->Add(this->button3);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->pictureBox1);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->textBox1);
			this->Controls->Add(this->label1);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
			this->MaximizeBox = false;
			this->Name = L"MainForm";
			this->Text = L"DEMO";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

		System::Void StopPlayThread();

		System::Threading::Thread^ threadPlay ;
		//置为false的时候，threadPlay会结束，必须声明为volatile
		volatile bool b_IsThreadPlayRunning ;

		System::Void StartPlayFile(System::String^ filename);
		System::Void threadRunDecodeAndPlay();
		//绘制帧方法
		System::Void drawFrame(Image^ imageToDraw);
		//用于绘制帧的委托
		delegate System::Void DrawFrameDelegate(Image^ imageToDraw);
		//用于绘制帧的委托实例
		DrawFrameDelegate^ RunDrawFrame;

	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
				 if( openFileDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK)
				 {
					 textBox1->Text = openFileDialog1->FileName;
					 try
					 {
						 
						 StartPlayFile(textBox1 -> Text);
					
					 }
					 catch(Exception^ ex)
					 {
						 MessageBox::Show(ex->ToString());
					 }

				 }
			 }





	private: System::Void button3_Click(System::Object^  sender, System::EventArgs^  e) {
				 StopPlayThread();
			 }
private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {
			 try
			 {
				StartPlayFile(textBox1->Text);
			 }
			 catch(Exception^ e)
			 {
				 MessageBox::Show(e->ToString());
			 }
		 }
};
}
