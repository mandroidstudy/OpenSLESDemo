#include <jni.h>
#include <string>
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,"OpenSLES",__VA_ARGS__)
// engine
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine = NULL;

// output mix
static const SLEnvironmentalReverbSettings reverbSettings =
        SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

//player
static SLObjectItf playerObject = NULL;
static SLPlayItf player;

//bufferQueueItf
static SLAndroidSimpleBufferQueueItf  bufferQueueItf;

FILE* file;
u_int8_t *buff;
void * pcmData;

size_t getPcmData(void **pcmData){
    while (feof(file)==0){
        size_t len=fread(buff,1,44100*2*2,file);
        if (len==0){
            LOGD("play end");
        } else{
            LOGD("playing");
        }
        (*pcmData)=buff;
        return len;
    }
    return static_cast<size_t>(-1);
}

//每次缓存队列的数据播放完就会回调这个函数
void bufferQueueCallable(SLAndroidSimpleBufferQueueItf caller,void *pContext){
    size_t len=getPcmData(&pcmData);
    if(pcmData&&len!=-1){
        (*bufferQueueItf)->Enqueue(bufferQueueItf,pcmData,len);
    } else{
        LOGD("读取结束");
        if(file!=NULL){
            fclose(file);
            file=NULL;
        }
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mao_openslesdemo_MainActivity_nativePlay(JNIEnv *env, jobject thiz, jstring url) {
    const char* c_url=env->GetStringUTFChars(url, 0);
    file=fopen(c_url,"r");
    if (!file){
        LOGD("can not open file:%s",c_url);
        return;
    }
    buff= static_cast<u_int8_t *>(malloc(44100 * 2*2));
    SLresult sLresult;
    //1、获取引擎接口
    slCreateEngine(&engineObject,0, NULL, 0, NULL, NULL);
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    //2、创建输出混音器
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    sLresult=(*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                     &outputMixEnvironmentalReverb);
    if (sLresult==SL_RESULT_SUCCESS){
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void)sLresult;
    }

    //3、创建播放器
    //创建播放器需要为其指定Data Source 和 Data Sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq={
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            2
    };
    //Data Source
    SLDataFormat_PCM format_pcm={
            SL_DATAFORMAT_PCM,//是pcm格式的
            2,//两声道
            SL_SAMPLINGRATE_44_1,//每秒的采样率
            SL_PCMSAMPLEFORMAT_FIXED_16,//每个采样的位数
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一样就行
            SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,//立体声(左前和右前)
            SL_BYTEORDER_LITTLEENDIAN,//播放结束的标志
    };
    SLDataSource dataSource={&loc_bufq,&format_pcm};
    //Data Sink
    SLDataLocator_OutputMix loc_outmix={SL_DATALOCATOR_OUTPUTMIX,outputMixObject};
    SLDataSink dataSink={&loc_outmix,NULL};

    //create audio player:
    const SLInterfaceID iids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean ireq[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    (*engineEngine)->CreateAudioPlayer(engineEngine,&playerObject,
            &dataSource,&dataSink,3,iids,ireq);
    (*playerObject)->Realize(playerObject,SL_BOOLEAN_FALSE);
    //获取播放接口
    (*playerObject)->GetInterface(playerObject,SL_IID_PLAY,&player);

    env->ReleaseStringUTFChars( url, c_url);

    //4、获取缓冲队列接口、给缓冲队列注册回调函数
    (*playerObject)->GetInterface(playerObject,SL_IID_BUFFERQUEUE,&bufferQueueItf);
    (*bufferQueueItf)->RegisterCallback(bufferQueueItf,bufferQueueCallable,NULL);

    //5、设置播放状态、主动调用一次回调使缓存队列接口工作
    (*player)->SetPlayState(player,SL_PLAYSTATE_PLAYING);
    bufferQueueCallable(bufferQueueItf,NULL);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mao_openslesdemo_MainActivity_release(JNIEnv *env, jobject thiz) {
    if (playerObject!=NULL){
        (*playerObject)->Destroy(playerObject);
        playerObject=NULL;
        bufferQueueItf=NULL;
    }

    if (outputMixObject!=NULL){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject=NULL;
        outputMixEnvironmentalReverb=NULL;
    }

    if (engineObject!=NULL){
        (*engineObject)->Destroy(engineObject);
        engineObject=NULL;
        engineEngine=NULL;
    }

    if(buff!=NULL){
        free(buff);
        buff=NULL;
    }
}