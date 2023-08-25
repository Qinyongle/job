#include "audio.h"
#include <Windows.h>
#include <atomic>
#include <condition_variable>
#include <QEventLoop>

//g_collectFrameCnt等于g_encodeFrameCnt证明编解码帧数一致
int g_collectFrameCnt = 0;	//采集帧数
int g_encodeFrameCnt = 0;	//编码帧数
static char *dup_wchar_to_utf8(wchar_t *w)
{
    char *s = NULL;
    int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
    s = (char *)av_malloc(l);
    if (s)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
    return s;

}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}


Audio::Audio(QObject *parent) : QObject(parent)
  ,m_aIndex(-1)
  ,m_state(RecordState::NotStarted)
  ,m_stop(false)
  ,m_bitrate(128000)
{

}

bool Audio::checkMicName()
{
    avdevice_register_all();    //注册
     const AVInputFormat* iFormat = av_find_input_format("dshow");
     AVDeviceInfoList *devlist=nullptr;
     avdevice_list_input_sources(iFormat,nullptr,nullptr,&devlist);  //获取输入的所有设备

     enum AVMediaType* mediaType=nullptr;
     if(devlist!=nullptr){
         for(int i=0;i<devlist->nb_devices; i++)
         {
             mediaType=devlist->devices[i]->media_types;

             if(*mediaType==AVMEDIA_TYPE_AUDIO) {
                 micName=devlist->devices[i]->device_description;
                 qDebug()<<micName;
             }
         }
     }
     if(micName.isEmpty())  return false;

     return true;
}

bool Audio::recording()
{

    std::thread recordThread(&Audio::RecordAudioThreadProc,this);
    recordThread.detach();      //分离创建线程，不受主线程控制



   // av_register_all();
    avdevice_register_all();  // 注册所有设备
    avformat_network_init();
//     const AVInputFormat* iFormat = av_find_input_format("dshow");
//     AVFormatContext *ctx=nullptr;

//    QString audioDeviceName = "audio=" + micName;

//     const char *devName = "audio=麦克风阵列 (适用于数字麦克风的英特尔® 智音技术)";

//    qDebug()<<audioDeviceName.toStdString().c_str();
//     int ret = avformat_open_input(&ctx,devName,iFormat,nullptr);
//     qDebug()<<"ret="<<ret;
//     if(ret<0)
//     {
//         char errbuff[1024]={0};
//         av_strerror(ret,errbuff,sizeof (errbuff));
//         qDebug()<<"打开设备失败："<<errbuff;
//         return 0;
//     }

//     const char* filename="C:/Users/User/Desktop/output.pcm";
//     QFile file(filename);

//     qDebug()<<filename;

//     if(!file.open(QIODevice::WriteOnly))
//     {
//         qDebug()<<"open file failed "<<filename<<endl;
//         avformat_close_input(&ctx);
//         return 0;
//     }
//     AVPacket pkt;

//     int count = 50 ;
//     while(count-- >0 &&av_read_frame(ctx,&pkt)==0)
//     {
//         file.write((const char*)pkt.data,pkt.size);
//     }

//     avformat_close_input(&ctx);
//     file.close();
//     qDebug()<<"record end"<<endl;


     return 0;

}

void Audio::RecordAudioThreadProc()
{

    avdevice_register_all();  // 注册所有设备
    avformat_network_init();
    int aFrameIndex = 0;

    if(OpenAudio()<0)
        return ;
    if(OpenOutput()<0)
        return ;



        m_nbSamples = m_aEncodeCtx->frame_size;
        qDebug()<<" m_aEncodeCtx->frame_size: "<< m_aEncodeCtx->frame_size;
        if (!m_nbSamples)
        {
            qDebug() << "m_nbSamples==0";
            m_nbSamples = 1024;
        }
        m_aFifoBuf = av_audio_fifo_alloc(m_aEncodeCtx->sample_fmt, m_aEncodeCtx->channels, 30 * m_aDecodeCtx->sample_rate);
        if (!m_aFifoBuf)
        {
            qDebug() << "av_audio_fifo_alloc failed";
            return;
        }

        //启动音视频数据采集线程
            std::thread soundRecord(&Audio::AcquireSoundThreadProc, this);
            soundRecord.detach();

        while(1)
        {

            if(m_state==RecordState::Stopped)
            {
                std::lock_guard<std::mutex> lk(m_mtx);
                if(av_audio_fifo_size(m_aFifoBuf)<m_nbSamples)
                    break;
            }
            std::unique_lock<std::mutex> lk(m_mtx);
            m_cvNotEmpty.wait(lk,[this]{return av_audio_fifo_size(m_aFifoBuf) >= m_aEncodeCtx->frame_size;});


            int ret = -1;

             AVFrame *aFrame = av_frame_alloc();
             aFrame->nb_samples = m_aEncodeCtx->frame_size;
             aFrame->channels=m_aDecodeCtx->channels;
             aFrame->channel_layout = m_aDecodeCtx->channel_layout;
             aFrame->format = m_aEncodeCtx->sample_fmt;
             aFrame->sample_rate = m_aEncodeCtx->sample_rate;
             aFrame->pts = aFrameIndex * m_nbSamples;
             ++aFrameIndex;

             ret = av_frame_get_buffer(aFrame,0);

             av_audio_fifo_read(m_aFifoBuf,(void **)aFrame->data,m_aEncodeCtx->frame_size);
             m_cvNotFull.notify_one();

             AVPacket pkt ={0};

             av_init_packet(&pkt);
//                          qDebug()<<"m_nbSamples: "<<m_nbSamples;
//                          qDebug()<<"aFrame->nb_samples: "<<aFrame->nb_samples;
//                          qDebug()<<"m_aEncodeCtx->frame_size: "<<m_aEncodeCtx->frame_size;

             ret = avcodec_send_frame(m_aEncodeCtx,aFrame);

             if(ret!=0)
             {
                // qDebug() << "audio avcodec_send_frame failed, ret: " << ret;

                 av_frame_free(&aFrame);
                 av_packet_unref(&pkt);
                 continue;
             }


             ret=avcodec_receive_packet(m_aEncodeCtx,&pkt);
             if(ret<0)
             {
                 qDebug()<<"faile to receive pkt";
             }

             pkt.stream_index=m_aOutIndex;

             pkt.pts = aFrame->pts;
             pkt.dts = pkt.pts;
             pkt.duration = m_nbSamples;
             ret=av_write_frame(m_oFmtCtx,&pkt);
             //ret=av_interleaved_write_frame(m_oFmtCtx,&pkt);

             if(ret==0)
                qDebug()<<"Write audio packet id: "<<++g_encodeFrameCnt;

             else
                 qDebug() << "audio av_interleaved_write_frame failed, ret: " << ret;
             av_frame_free(&aFrame);
             av_packet_unref(&pkt);

        }

        FlushEncoder();
        av_write_trailer(m_oFmtCtx);
        Release();
        qDebug()<<"parent thread exit";

}

void Audio::AcquireSoundThreadProc()
{
    int ret = -1;
    AVPacket pkg ={0};

    av_init_packet(&pkg);

    int nbSamples=m_nbSamples;
    int dstNbSamples,maxDstNbSamples;

    AVFrame *rawFrame=av_frame_alloc();
    AVFrame *newFrame=AllocAudioFrame(m_aEncodeCtx,nbSamples);

    maxDstNbSamples=dstNbSamples=av_rescale_rnd(nbSamples,
        m_aEncodeCtx->sample_rate,m_aDecodeCtx->sample_rate,AV_ROUND_UP);

    while(m_state!=RecordState::Stopped)
    {
        if(av_read_frame(m_aFmtCtx,&pkg)<0)
        {
            qDebug() << "audio av_read_frame < 0";
            continue;
        }
        if(pkg.stream_index!=m_aIndex)
        {
            av_packet_unref(&pkg);
            continue;
        }
        ret=avcodec_send_packet(m_aDecodeCtx,&pkg);
        if(ret!=0)
        {
            av_packet_unref(&pkg);
            continue;
        }
        ret=avcodec_receive_frame(m_aDecodeCtx,rawFrame);
        if(ret!=0)
        {
            av_packet_unref(&pkg);
            continue;
        }

        dstNbSamples=av_rescale_rnd(swr_get_delay(m_swrCtx,m_aDecodeCtx->sample_rate)+rawFrame->nb_samples,
            m_aEncodeCtx->sample_rate,m_aDecodeCtx->sample_rate,AV_ROUND_UP);

        qDebug()<<"dstNbSamples: "<<dstNbSamples;
        qDebug()<<"maxDstNbSamples: "<<maxDstNbSamples;
        if(dstNbSamples>maxDstNbSamples)
        {
            qDebug()<<">>>";
            av_freep(&newFrame->data[0]);
            ret=av_samples_alloc(newFrame->data,newFrame->linesize,m_aEncodeCtx->channels,
                dstNbSamples,m_aEncodeCtx->sample_fmt,1);
            if(ret<0)
            {
                qDebug()<<"av_sample alloc failed";
                return;
            }
            maxDstNbSamples=dstNbSamples;
            m_aEncodeCtx->frame_size=dstNbSamples;
            m_nbSamples=newFrame->nb_samples;
        }


        newFrame->nb_samples = swr_convert(m_swrCtx, newFrame->data, dstNbSamples,
            (const uint8_t **)rawFrame->data, rawFrame->nb_samples);
        if (newFrame->nb_samples < 0)
        {
            qDebug() << "swr_convert failed";
            return;
        }
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            m_cvNotFull.wait(lk, [newFrame, this] { return av_audio_fifo_space(m_aFifoBuf) >= newFrame->nb_samples; });
        }
        if (av_audio_fifo_write(m_aFifoBuf, (void **)newFrame->data, newFrame->nb_samples) < newFrame->nb_samples)
        {
            qDebug() << "av_audio_fifo_write";
            return;
        }
        //m_nbSamples = newFrame->nb_samples;
        m_cvNotEmpty.notify_one();


     }

    av_frame_free(&rawFrame);
    av_frame_free(&newFrame);
    qDebug() << "sound record thread exit";
}

int Audio::OpenAudio()
{

 const AVCodec *decoder;
    const AVInputFormat* iFormat = av_find_input_format("dshow");
    AVFormatContext *ctx=nullptr;

   QString audioDeviceName = "audio=" + micName;

    const char *devName = "audio=麦克风阵列 (适用于数字麦克风的英特尔® 智音技术)";

   qDebug()<<audioDeviceName.toStdString().c_str();
    int ret = avformat_open_input(&m_aFmtCtx,devName,iFormat,nullptr);
    qDebug()<<"ret="<<ret;
    if(ret<0)
    {
        char errbuff[1024]={0};
        av_strerror(ret,errbuff,sizeof (errbuff));
        qDebug()<<"打开设备失败："<<errbuff;
        return -1;
    }

    if(avformat_find_stream_info(m_aFmtCtx,nullptr)<0)
        return -1;


    /*遍历流信息，找到音频流*/
    for(int i=0;i<m_aFmtCtx->nb_streams;i++)
    {
        qDebug()<<"device: "<<i;
        if(m_aFmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            qDebug()<<"coede type"<<m_aFmtCtx->streams[i]->codecpar->codec_type;
            /*打开解码器*/
            decoder=avcodec_find_decoder(m_aFmtCtx->streams[i]->codecpar->codec_id);

            if(decoder==nullptr)
            {
                qDebug()<<QString::fromLocal8Bit("没有找到解码器");
                return -1;
            }


            m_aDecodeCtx=avcodec_alloc_context3(decoder);

            ret=avcodec_parameters_to_context(m_aDecodeCtx,m_aFmtCtx->streams[i]->codecpar);
            qDebug()<<"ret: "<<ret;
            if(ret<0)
            {
                qDebug()<<"Parameter to contexts failed,err code: "<<ret;
                return -1;
            }
            m_aIndex=i;
            break;
        }

    }

    if(avcodec_open2(m_aDecodeCtx,decoder,NULL)<0)
    {
        qDebug()<<"can not find or open audio decoder!";
        return -1;
    }

qDebug()<<__func__<<__LINE__;
    return 0;

}

int Audio::OpenOutput()
{
    AVStream *vStream=nullptr,*aStream=nullptr;

    std::string filePath=m_filePath.toStdString();

    int ret =avformat_alloc_output_context2(&m_oFmtCtx,nullptr,nullptr,filePath.c_str());

    if (ret<0)
    {
        qDebug()<<__func__<<" failed";
        return -1;
    }

    if(m_aFmtCtx->streams[m_aIndex]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO)
    {

        aStream=avformat_new_stream(m_oFmtCtx,NULL);
        if(!aStream)
        {
            qDebug()<<"can not new audio stream for output";
            return -1;
        }

        m_aOutIndex=aStream->index;

        const AVCodec *encoder = avcodec_find_encoder(m_oFmtCtx->oformat->audio_codec);
        if(!encoder)
        {
            qDebug()<<"can not find audio encoder,id : "<<m_oFmtCtx->oformat->audio_codec;
            return -1;
        }
        m_aEncodeCtx=avcodec_alloc_context3(encoder);
        if(m_aEncodeCtx==nullptr)
        {
            qDebug()<<__func__<<"failed";
            return -1;
        }

        //采样音频格式、采样率
        m_aEncodeCtx->sample_fmt=encoder->sample_fmts?encoder->sample_fmts[0]:AV_SAMPLE_FMT_FLTP;
        m_aEncodeCtx->bit_rate=m_bitrate;
        m_aEncodeCtx->sample_rate=44100;
        if(encoder->supported_samplerates)
        {
            m_aEncodeCtx->sample_rate=encoder->supported_samplerates[0];
            for(int i=0;encoder->supported_samplerates[i];i++)
            {
                if(encoder->supported_samplerates[i]==44100)
                    m_aEncodeCtx->sample_rate=44100;
            }

        }

        //通道设置
        m_aEncodeCtx->channels = av_get_channel_layout_nb_channels(m_aEncodeCtx->channel_layout);
        m_aEncodeCtx->channel_layout = AV_CH_LAYOUT_STEREO;
        if (encoder->channel_layouts)
                {
                    m_aEncodeCtx->channel_layout = encoder->channel_layouts[0];
                    for (int i = 0; encoder->channel_layouts[i]; ++i)
                    {
                        if (encoder->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                            m_aEncodeCtx->channel_layout = AV_CH_LAYOUT_STEREO;
                    }
                }
         m_aEncodeCtx->channels = av_get_channel_layout_nb_channels(m_aEncodeCtx->channel_layout);

        m_aEncodeCtx->time_base = AVRational{1,m_aEncodeCtx->sample_rate};
        aStream->time_base=AVRational{1,m_aEncodeCtx->sample_rate};

        m_aEncodeCtx->codec_tag=0;

        m_aEncodeCtx->flags|=AV_CODEC_FLAG_GLOBAL_HEADER;


    if(!check_sample_fmt(encoder,m_aEncodeCtx->sample_fmt))
    {
        qDebug() << "Encoder does not support sample format " << av_get_sample_fmt_name(m_aEncodeCtx->sample_fmt);
        return -1;

    }

   int ret= avcodec_open2(m_aEncodeCtx,encoder,0);

   if(ret<0)
   {

       qDebug() << "Can not open the audio encoder, id: " << encoder->id << "error code: " << ret;
       return -1;

   }

   ret = avcodec_parameters_from_context(aStream->codecpar,m_aEncodeCtx);

   if (ret < 0)
   {
       qDebug() << "Output audio avcodec_parameters_from_context,error code:" << ret;
       return -1;
   }

   m_swrCtx=swr_alloc();

   if(!m_swrCtx)
   {
       qDebug() << "swr_alloc failed";
       return -1;
   }

   av_opt_set_int(m_swrCtx,"in_channel_count",m_aDecodeCtx->channels,0);
   av_opt_set_int(m_swrCtx,"in_sample_rate",m_aDecodeCtx->sample_rate,0);
   av_opt_set_sample_fmt(m_swrCtx,"in_sample_fmt",m_aDecodeCtx->sample_fmt,0);
   av_opt_set_int(m_swrCtx,"out_channel_count",m_aEncodeCtx->channels,0);
   av_opt_set_int(m_swrCtx,"out_sample_rate",m_aEncodeCtx->sample_rate,0);
   av_opt_set_sample_fmt(m_swrCtx,"out_sample_fmt",m_aEncodeCtx->sample_fmt,0);
   if((ret=swr_init(m_swrCtx))<0)
   {
       qDebug()<<"swr_init failed";
       return -1;
   }
 }
    if(!(m_oFmtCtx->oformat->flags&AVFMT_NOFILE))
    {
       if(avio_open(&m_oFmtCtx->pb,filePath.c_str(),AVIO_FLAG_WRITE)<0)
       {
           qDebug()<<"can not open output file";
           return -1;
       }
    }
    if(avformat_write_header(m_oFmtCtx,nullptr)<0)
    {
        printf("can not write the hearder of  the output file\n");
        return -1;
    }

    return 0;
}

AVFrame *Audio::AllocAudioFrame(AVCodecContext *c, int nbSamples)
{
    AVFrame *frame = av_frame_alloc();
        int ret;

        frame->format = c->sample_fmt;
        frame->channel_layout = c->channel_layout ? c->channel_layout: AV_CH_LAYOUT_STEREO;
        frame->sample_rate = c->sample_rate;
        frame->nb_samples = nbSamples;

        if (nbSamples)
        {
            ret = av_frame_get_buffer(frame, 0);
            if (ret < 0)
            {
                qDebug() << "av_frame_get_buffer failed";
                return nullptr;
            }
        }
        return frame;
}

void Audio::FlushEncoder()
{
    int ret = -1;
        int nFlush = 0;
        AVPacket pkt = { 0 };
        av_init_packet(&pkt);
        ret = avcodec_send_frame(m_aEncodeCtx, nullptr);
        qDebug() << "flush audio avcodec_send_frame ret: " << ret;
 while (ret >= 0)
 {
  ret = avcodec_receive_packet(m_aEncodeCtx, &pkt);
    if (ret < 0)
    {
        av_packet_unref(&pkt);
        if (ret == AVERROR(EAGAIN))
        {
            qDebug() << "flush EAGAIN avcodec_receive_packet";
            ret = 1;
            continue;
        }
        else if (ret == AVERROR_EOF)
        {
            qDebug() << "flush video encoder finished";
            break;
        }
        qDebug() << "flush audio avcodec_receive_packet failed, ret: " << ret;
        return;
    }
    ++nFlush;
    pkt.stream_index = m_aOutIndex;
    ret = av_interleaved_write_frame(m_oFmtCtx, &pkt);
    if (ret == 0)
        qDebug() << "flush write audio packet id: " << ++g_encodeFrameCnt;
    else
        qDebug() << "flush audio av_interleaved_write_frame failed, ret: " << ret;
    av_packet_unref(&pkt);
}
 qDebug() << "flush times: " << nFlush;
}

void Audio::Release()
{
    if (m_oFmtCtx)
        {
            avio_close(m_oFmtCtx->pb);
            avformat_free_context(m_oFmtCtx);
            m_oFmtCtx = nullptr;
        }
        if (m_aDecodeCtx)
        {
            avcodec_free_context(&m_aDecodeCtx);
            m_aDecodeCtx = nullptr;
        }
        if (m_aEncodeCtx)
        {
            avcodec_free_context(&m_aEncodeCtx);
            m_aEncodeCtx = nullptr;
        }
        if (m_aFifoBuf)
        {
            av_audio_fifo_free(m_aFifoBuf);
            m_aFifoBuf = nullptr;
        }
        if (m_aFmtCtx)
        {
            avformat_close_input(&m_aFmtCtx);
            m_aFmtCtx = nullptr;
        }
}

void Audio::delay(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms,&loop,SLOT(quit()));
    loop.exec();
}

void Audio::audioRecord()
{

//    qDebug()<<__FUNCTION__;
//    if(checkMicName()==true)
//    {
//        qDebug()<<"true";
//    }
   // recording();

    stateChange();
}

int Audio::stateChange()
{
    switch(m_state)
    {
        case NotStarted:
    {
        m_state=RecordState::Started;
        qDebug()<<"start record";

        std::thread recordThread(&Audio::RecordAudioThreadProc,this);
        recordThread.detach();      //分离创建线程，不受主线程控制
    }break;
        case Started:m_state=RecordState::Stopped;qDebug()<<"stop record";break;
        case Stopped:m_state=RecordState::Started;qDebug()<<"start record";break;
        default:;
    }
    return 0;
}
