#include "AudioInput.h"
#include "netheader.h"
#include <QAudioFormat>
#include <QDebug>
#include <QThread>
//申明发送和接收信息的队列
extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;

AudioInput::AudioInput(QObject *parent)
	: QObject(parent)
{
    // 分配接收缓冲区内存
	recvbuf = (char*)malloc(MB * 2);
	QAudioFormat format;
	//set format
    format.setSampleRate(8000);// 设置采样率为8000Hz
    format.setChannelCount(1); // 设置单声道
    format.setSampleRate(16);// 设置采样大小为16位
    format.setCodec("audio/pcm");// 设置编解码器为PCM
    format.setByteOrder(QAudioFormat::LittleEndian);// 设置字节序为小端
    format.setSampleType(QAudioFormat::UnSignedInt);// 设置样本类型为无符号整数


    // 获取默认音频输入设备信息
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
	if (!info.isFormatSupported(format))
	{
        qWarning() << "Default format not supported, trying to use the nearest.默认格式不支持，尝试使用最接近的格式。";
		format = info.nearestFormat(format);
	}
    // 创建音频输入对象
	audio = new QAudioInput(format, this);
    // 连接音频状态改变信号和处理状态改变的槽函数
	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));

}

AudioInput::~AudioInput()
{
    // 析构函数，释放音频输入对象
	delete audio;
}

void AudioInput::startCollect()
{
    // 开始采集音频
    if (audio->state() == QAudio::ActiveState) return;
	WRITE_LOG("start collecting audio");
	inputdevice = audio->start();
	connect(inputdevice, SIGNAL(readyRead()), this, SLOT(onreadyRead()));
}

void AudioInput::stopCollect()
{
    // 停止采集音频
	if (audio->state() == QAudio::StoppedState) return;
	disconnect(this, SLOT(onreadyRead()));
	audio->stop();
	WRITE_LOG("stop collecting audio");
	inputdevice = nullptr;
}

void AudioInput::onreadyRead()
{
    // 音频数据就绪可读时的处理函数
	static int num = 0, totallen  = 0;
	if (inputdevice == nullptr) return;
	int len = inputdevice->read(recvbuf + totallen, 2 * MB - totallen);
	if (num < 2)
	{
		totallen += len;
		num++;
		return;
	}
	totallen += len;
	qDebug() << "totallen = " << totallen;

    // 创建消息对象
	MESG* msg = (MESG*)malloc(sizeof(MESG));
	if (msg == nullptr)
	{
        qWarning() << __LINE__ << "malloc fail内存分配失败";
	}
	else
	{
		memset(msg, 0, sizeof(MESG));
		msg->msg_type = AUDIO_SEND;
		//压缩数据，转base64
		QByteArray rr(recvbuf, totallen);
		QByteArray cc = qCompress(rr).toBase64();
		msg->len = cc.size();

		msg->data = (uchar*)malloc(msg->len);
		if (msg->data == nullptr)
		{
			qWarning() << "malloc mesg.data fail";
		}
		else
		{
			memset(msg->data, 0, msg->len);
			memcpy_s(msg->data, msg->len, cc.data(), cc.size());
            // 将消息压入发送队列
			queue_send.push_msg(msg);
		}
	}
	totallen = 0;
	num = 0;
}

QString AudioInput::errorString()
{
	if (audio->error() == QAudio::OpenError)
	{
		return QString("AudioInput An error occurred opening the audio device").toUtf8();
	}
	else if (audio->error() == QAudio::IOError)
	{
		return QString("AudioInput An error occurred during read/write of audio device").toUtf8();
	}
	else if (audio->error() == QAudio::UnderrunError)
	{
		return QString("AudioInput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
	}
	else if (audio->error() == QAudio::FatalError)
	{
		return QString("AudioInput A non-recoverable error has occurred, the audio device is not usable at this time.");
	}
	else
	{
		return QString("AudioInput No errors have occurred").toUtf8();
	}
}

void AudioInput::handleStateChanged(QAudio::State newState)
{
    // 处理音频状态改变
	switch (newState)
	{
		case QAudio::StoppedState:
			if (audio->error() != QAudio::NoError)
			{
				stopCollect();
				emit audioinputerror(errorString());
			}
			else
			{
				qWarning() << "stop recording";
			}
			break;
		case QAudio::ActiveState:
			//start recording
			qWarning() << "start recording";
			break;
		default:
			//
			break;
	}
}

void AudioInput::setVolumn(int v)
{
    // 设置音频音量
	qDebug() << v;
	audio->setVolume(v / 100.0);
}
