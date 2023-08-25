#ifndef AUDIO_H
#define AUDIO_H

#include <QObject>
#include <QProcess>
#include <QDebug>
#include <common.h>
#include <mutex>


extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include <libavutil\avassert.h>


}


#ifdef	__cplusplus
extern "C"
{
#endif
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFifoBuffer;
struct AVAudioFifo;
struct AVFrame;
struct SwsContext;
struct SwrContext;
#ifdef __cplusplus
};
#endif

class Audio : public QObject
{
    Q_OBJECT
public:
    explicit Audio(QObject *parent = nullptr);
    bool checkMicName(void);
    bool recording();


    QString micName= QString::fromLocal8Bit( "��˷����� (������������˷��Ӣ�ض� ��������)");
    uint16_t sampleRate=16000;
    uint8_t audioChannels=1;
    int stateChange();

private:
    enum RecordState {
            NotStarted,
            Started,
            Paused,
            Stopped,
            Unknown,
        };




    //��fifobuf��ȡ��Ƶ֡�����룬д��������������ļ�
    void RecordAudioThreadProc();
    //����Ƶ��������ȡ֡��д��fifobuf
    void AcquireSoundThreadProc(void);
    int OpenAudio();
    int OpenOutput();
    QString GetSpeakerDeviceName();
    QString GetMicrophoneDeviceName();
    AVFrame* AllocAudioFrame(AVCodecContext* c, int nbSamples);
    //ȡ�����������֡��д�������
    void FlushEncoder();
    void Release();
    void delay(int);

private:
    QString						m_filePath="C:/Users/User/Desktop/output.wav";
    int							m_bitrate;
    int							m_aIndex;	//������Ƶ������
    int							m_aOutIndex;//�����Ƶ������
    AVFormatContext				*m_aFmtCtx=nullptr;
    AVFormatContext				*m_oFmtCtx;
    AVCodecContext				*m_aDecodeCtx;
    AVCodecContext				*m_aEncodeCtx;
    SwrContext					*m_swrCtx;
    AVAudioFifo					*m_aFifoBuf;

    std::atomic_bool			m_stop;


        int							m_nbSamples;
        std::mutex					m_mtx;
        std::condition_variable		m_cvNotEmpty;	//��fifoBuf���ˣ������̹߳���
        std::condition_variable		m_cvNotFull;	//��fifoBuf���ˣ��ɼ��̹߳���
        RecordState					m_state;

signals:
        void recordStart();

public slots:
        void audioRecord(void);


};

#endif // AUDIO_H
