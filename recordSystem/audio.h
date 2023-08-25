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


    QString micName= QString::fromLocal8Bit( "麦克风阵列 (适用于数字麦克风的英特尔 智音技术)");
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




    //从fifobuf读取音频帧，编码，写入输出流，生成文件
    void RecordAudioThreadProc();
    //从音频输入流读取帧，写入fifobuf
    void AcquireSoundThreadProc(void);
    int OpenAudio();
    int OpenOutput();
    QString GetSpeakerDeviceName();
    QString GetMicrophoneDeviceName();
    AVFrame* AllocAudioFrame(AVCodecContext* c, int nbSamples);
    //取出编码器里的帧，写入输出流
    void FlushEncoder();
    void Release();
    void delay(int);

private:
    QString						m_filePath="C:/Users/User/Desktop/output.wav";
    int							m_bitrate;
    int							m_aIndex;	//输入音频流索引
    int							m_aOutIndex;//输出音频流索引
    AVFormatContext				*m_aFmtCtx=nullptr;
    AVFormatContext				*m_oFmtCtx;
    AVCodecContext				*m_aDecodeCtx;
    AVCodecContext				*m_aEncodeCtx;
    SwrContext					*m_swrCtx;
    AVAudioFifo					*m_aFifoBuf;

    std::atomic_bool			m_stop;


        int							m_nbSamples;
        std::mutex					m_mtx;
        std::condition_variable		m_cvNotEmpty;	//当fifoBuf空了，编码线程挂起
        std::condition_variable		m_cvNotFull;	//当fifoBuf满了，采集线程挂起
        RecordState					m_state;

signals:
        void recordStart();

public slots:
        void audioRecord(void);


};

#endif // AUDIO_H
