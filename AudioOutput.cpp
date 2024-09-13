#include "AudioOutput.h"
#include <QMutexLocker>
#include "netheader.h"
#include <QDebug>
#include <QHostAddress>

// 定义125ms音频帧的长度常量
#ifndef FRAME_LEN_125MS
#define FRAME_LEN_125MS 1900
#endif
// 外部音频接收队列
extern QUEUE_DATA<MESG> audio_recv; //音频接收队列

AudioOutput::AudioOutput(QObject *parent)
	: QThread(parent)
{
	QAudioFormat format;
    // 设置音频格式
	format.setSampleRate(8000);
	format.setChannelCount(1);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::UnSignedInt);

    // 获取默认音频输出设备信息
	QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
	if (!info.isFormatSupported(format))
	{
		qWarning() << "Raw audio format not supported by backend, cannot play audio.";
		return;
	}
    // 创建音频输出对象
	audio = new QAudioOutput(format, this);
        // 连接音频状态改变信号和处理状态改变的槽函数
	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State) ));
	outputdevice = nullptr;
}

AudioOutput::~AudioOutput()
{
	delete audio;
}

QString AudioOutput::errorString()
{
	if (audio->error() == QAudio::OpenError)
	{
		return QString("AudioOutput An error occurred opening the audio device").toUtf8();
	}
	else if (audio->error() == QAudio::IOError)
	{
		return QString("AudioOutput An error occurred during read/write of audio device").toUtf8();
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
		return QString("AudioOutput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
	}
	else if (audio->error() == QAudio::FatalError)
	{
		return QString("AudioOutput A non-recoverable error has occurred, the audio device is not usable at this time.");
	}
	else
	{
		return QString("AudioOutput No errors have occurred").toUtf8();
	}
}

void AudioOutput::handleStateChanged(QAudio::State state)// 处理音频状态改变
{
	switch (state)
	{
		case QAudio::ActiveState:
			break;
		case QAudio::SuspendedState:
			break;
		case QAudio::StoppedState:
			if (audio->error() != QAudio::NoError)
			{
				audio->stop();
				emit audiooutputerror(errorString());
				qDebug() << "out audio error" << audio->error();
			}
			break;
		case QAudio::IdleState:
			break;
		case QAudio::InterruptedState:
			break;
		default:
			break;
	}
}

void AudioOutput::startPlay()// 开始播放音频
{
	if (audio->state() == QAudio::ActiveState) return;
	WRITE_LOG("start playing audio");
	outputdevice = audio->start();
}

void AudioOutput::stopPlay()// 停止播放音频
{
	if (audio->state() == QAudio::StoppedState) return;
	{
		QMutexLocker lock(&device_lock);
		outputdevice = nullptr;
	}
	audio->stop();
	WRITE_LOG("stop playing audio");
}

void AudioOutput::run()//音频播放线程运行函数
{
	is_canRun = true;
	QByteArray m_pcmDataBuffer;

	WRITE_LOG("start playing audio thread 0x%p", QThread::currentThreadId());
	for (;;)
	{
		{
			QMutexLocker lock(&m_lock);
			if (is_canRun == false)
			{
				stopPlay();
				WRITE_LOG("stop playing audio thread 0x%p", QThread::currentThreadId());
				return;
			}
		}
		MESG* msg = audio_recv.pop_msg();
		if (msg == NULL) continue;
		{
			QMutexLocker lock(&device_lock);
			if (outputdevice != nullptr)
			{
				m_pcmDataBuffer.append((char*)msg->data, msg->len);

				if (m_pcmDataBuffer.size() >= FRAME_LEN_125MS)
				{
					//写入音频数据
					qint64 ret = outputdevice->write(m_pcmDataBuffer.data(), FRAME_LEN_125MS);
					if (ret < 0)
					{
						qDebug() << outputdevice->errorString();
						return;
					}
					else
					{
						emit speaker(QHostAddress(msg->ip).toString());
						m_pcmDataBuffer = m_pcmDataBuffer.right(m_pcmDataBuffer.size() - ret);
					}
				}
			}
			else
			{
				m_pcmDataBuffer.clear();
			}
		}
		if (msg->data) free(msg->data);
		if (msg) free(msg);
	}
}
// 立即停止播放
void AudioOutput::stopImmediately()
{
	QMutexLocker lock(&m_lock);
    is_canRun = false;// 设置运行标志为false，停止线程
}


void AudioOutput::setVolumn(int val)
{
	audio->setVolume(val / 100.0);
}

void AudioOutput::clearQueue()
{
	qDebug() << "audio recv clear";
	audio_recv.clear();
}
