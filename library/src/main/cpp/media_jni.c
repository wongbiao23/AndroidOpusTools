/*
 The MIT License (MIT)

Copyright (c) 2017-2020 oarplayer(qingkouwei)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#define _JNILOG_TAG "media_jni"

#include <assert.h>
#include <pthread.h>
#include <malloc.h>
#include "_android.h"
#include "jni_utils.h"
#include "libopus/include/opus.h"
#include "libopus/include/opus_multistream.h"


#define JNI_CLASS_LJMedia     "com/wodekouwei/sdk/library/OpusUtil"
#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif


typedef struct media_fields_t {
    pthread_mutex_t mutex;
    jclass clazz;
} media_fields_t;
static media_fields_t g_clazz;

static jlong
createOpusEncoder(JNIEnv *env, jobject thiz, jint sampleRateInHz, jint channelConfig, jint bitrate,
                  jint complexity) {
    int error;
    OpusEncoder *pOpusEnc = opus_encoder_create(sampleRateInHz, channelConfig,
                                                OPUS_APPLICATION_RESTRICTED_LOWDELAY,
                                                &error);
    if (pOpusEnc) {
        opus_encoder_ctl(pOpusEnc, OPUS_SET_VBR(0));//0:CBR, 1:VBR
        opus_encoder_ctl(pOpusEnc, OPUS_SET_VBR_CONSTRAINT(true));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_BITRATE(bitrate * 1000));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_COMPLEXITY(complexity));//8    0~10
        opus_encoder_ctl(pOpusEnc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_LSB_DEPTH(16));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_DTX(0));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_INBAND_FEC(0));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_PACKET_LOSS_PERC(0));
    }
    return (jlong) pOpusEnc;
}

static jint encodeOpus(JNIEnv *env, jobject thiz, jlong pOpusEnc, jshortArray samples, jint offset,
         jbyteArray bytes) {
    OpusEncoder *pEnc = (OpusEncoder *) pOpusEnc;
    if (!pEnc || !samples || !bytes)
        return 0;
    jshort *pSamples = (*env)->GetShortArrayElements(env, samples, 0);
    jsize nSampleSize = (*env)->GetArrayLength(env, samples);
    jbyte *pBytes = (*env)->GetByteArrayElements(env, bytes, 0);
    jsize nByteSize = (*env)->GetArrayLength(env, bytes);
    if (nSampleSize - offset < 320 || nByteSize <= 0)
        return 0;
    int nRet = opus_encode(pEnc, pSamples + offset, nSampleSize, (unsigned char *) pBytes,
                           nByteSize);
    (*env)->ReleaseShortArrayElements(env, samples, pSamples, 0);
    (*env)->ReleaseByteArrayElements(env, bytes, pBytes, 0);
    return nRet;
}

static void destroyOpusEncoder(JNIEnv *env, jobject thiz, jlong pOpusEnc) {
    OpusEncoder *pEnc = (OpusEncoder *) pOpusEnc;
    if (!pEnc)
        return;
    opus_encoder_destroy(pEnc);
}

static jlong createOpusDecoder(JNIEnv * env, jobject thiz, jint sampleRate, jint channel)
{
    int error;
    OpusDecoder * pOpusDec = opus_decoder_create(sampleRate, channel, &error);
    return (jlong)pOpusDec;
}

static jint decodeOpus(JNIEnv * env, jobject thiz, jlong decoder,
        jbyteArray encData, jbyteArray decBuf, jint frameSize)
{
    OpusDecoder * pOpusDec = (OpusDecoder *)decoder;
    if (!pOpusDec || !encData || !decBuf)
        return 0;

    jbyte * pEncData = (*env)->GetByteArrayElements(env, encData, 0);
    jsize encLen = (*env)->GetArrayLength(env, encData);
    jbyte * pDecBuf = (*env)->GetByteArrayElements(env, decBuf, 0);
    jsize decLen = (*env)->GetArrayLength(env, decBuf);

    jshort * decoded = (jshort *)malloc((size_t)decLen);

    jint res = opus_decode(pOpusDec, (unsigned char *)pEncData, encLen, decoded, frameSize, 0);
    jint channel = 1;
    LOGI("opus decode sample count %d.", res);
    if (res > 0) {
        jint i;
        for (i = 0; i < res * channel; i++) {
            pDecBuf[2*i] = (jbyte)(decoded[i] & 0xFF);
            pDecBuf[2*i+1] = (jbyte)((decoded[i] >> 8) & 0xFF);
        }
    }
    free(decoded);
    (*env)->ReleaseByteArrayElements(env, encData, pEncData, 0);
    (*env)->ReleaseByteArrayElements(env, decBuf, pDecBuf, 0);

    return res > 0 ? (res * 2 * channel) : res;
}

static void destroyOpusDecoder(JNIEnv * env, jobject thiz, jlong decoder)
{
    OpusDecoder * pOpusDecoder = (OpusDecoder *)decoder;
    if (!decoder) {
        return;
    }
    opus_decoder_destroy(pOpusDecoder);
}

static jlong
createOpusMSEncoder(JNIEnv *env, jobject thiz, jint sampleRateInHz, jint channelConfig, jint bitrate,
                  jint complexity) {
    int error;
    opus_int32 Fs = sampleRateInHz;
    int mapping_family = 1,
            streams = 0,
            coupled_streams = 0;
    unsigned char mapping[256];
    OpusMSEncoder * enc = opus_multistream_surround_encoder_create(Fs, channelConfig, mapping_family, \
        &streams, &coupled_streams, mapping, OPUS_APPLICATION_AUDIO, &error);
    if (enc) {
        opus_int32 enc_bitrate_bps = bitrate;
        opus_int32 enc_bandwidth = OPUS_AUTO;
        opus_int32 enc_use_vbr = 0;
        opus_int32 enc_cvbr = 0;
        opus_int32 enc_complexity = complexity;
        opus_int32 enc_use_inbandfec = 0;
        opus_int32 enc_forcechannels = OPUS_AUTO;
        opus_int32 enc_use_dtx = 0;
        opus_int32 enc_packet_loss_perc = 0;
        opus_int32 enc_skip;
        opus_int32 enc_depth = 16;
        opus_multistream_encoder_ctl(enc, OPUS_SET_BITRATE(enc_bitrate_bps * channelConfig)); // enc_bitrate_bps = 32000
        opus_multistream_encoder_ctl(enc, OPUS_SET_BANDWIDTH(enc_bandwidth)); // OPUS_AUTO
        opus_multistream_encoder_ctl(enc, OPUS_SET_VBR(enc_use_vbr)); // enc_use_vbr = 0
        opus_multistream_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(enc_cvbr)); // enc_cvbr = 0
        opus_multistream_encoder_ctl(enc, OPUS_SET_COMPLEXITY(enc_complexity)); // enc_complexity = 10
        opus_multistream_encoder_ctl(enc, OPUS_SET_INBAND_FEC(enc_use_inbandfec)); // enc_use_inbandfec = 0
        opus_multistream_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(enc_forcechannels)); // enc_forcechannels = OPUS_AUTO
        opus_multistream_encoder_ctl(enc, OPUS_SET_DTX(enc_use_dtx)); // enc_use_dtx = 0
        opus_multistream_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(enc_packet_loss_perc)); // enc_packet_loss_perc = 0
        opus_multistream_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&enc_skip)); //
        opus_multistream_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(enc_depth)); // enc_depth = 16
    }
    return (jlong) enc;
}

static jint encodeOpusMS(JNIEnv *env, jobject thiz, jlong pOpusEnc, jshortArray samples, jint offset,
                       jbyteArray bytes) {
    OpusMSEncoder *pEnc = (OpusMSEncoder *) pOpusEnc;
    if (!pEnc || !samples || !bytes) {
        LOGE("Please check the input parameters");
        return 0;
    }
    // TODO: SET DEFAULT CHANNELS TO 4
    int channels = 4;
    jshort *pSamples = (*env)->GetShortArrayElements(env, samples, 0);
    jsize nSampleSize = (*env)->GetArrayLength(env, samples)/channels;
    jbyte *pBytes = (*env)->GetByteArrayElements(env, bytes, 0);
    jsize nByteSize = (*env)->GetArrayLength(env, bytes);
    if (nSampleSize - offset < 160 || nByteSize <= 0)
        return 0;
    int nRet = opus_multistream_encode(pEnc, pSamples + offset, nSampleSize, \
                                       (unsigned char *) pBytes, nByteSize);
    (*env)->ReleaseShortArrayElements(env, samples, pSamples, 0);
    (*env)->ReleaseByteArrayElements(env, bytes, pBytes, 0);
    return nRet;
}

static void destroyOpusMSEncoder(JNIEnv *env, jobject thiz, jlong pOpusEnc) {
    OpusMSEncoder *pEnc = (OpusMSEncoder *) pOpusEnc;
    if (!pEnc)
        return;
    opus_multistream_encoder_destroy(pEnc);
}

static JNINativeMethod g_methods[] = {
        {"_createOpusEncoder",  "(IIII)J", (void *) createOpusEncoder},
        {"_encodeOpus",         "(J[SI[B)I",                 (void *) encodeOpus},
        {"_destroyOpusEncoder", "(J)V",                      (void *) destroyOpusEncoder},
        {"_createOpusDecoder",  "(II)J", (void *) createOpusDecoder},
        {"_decodeOpus",         "(J[B[BI)I",                 (void *) decodeOpus},
        {"_destroyOpusDecoder", "(J)V",                      (void *) destroyOpusDecoder},
        {"_createOpusMSEncoder",  "(IIII)J", (void *) createOpusMSEncoder},
        {"_encodeOpusMS",         "(J[SI[B)I",                 (void *) encodeOpusMS},
        {"_destroyOpusMSEncoder", "(J)V",                      (void *) destroyOpusMSEncoder},
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;

    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    pthread_mutex_init(&g_clazz.mutex, NULL);

    // FindClass returns LocalReference
    LJ_FIND_JAVA_CLASS(env, g_clazz.clazz, JNI_CLASS_LJMedia);
    (*env)->RegisterNatives(env, g_clazz.clazz, g_methods, NELEM(g_methods));


    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved) {
    LOGE("JNI_OnUnload....");
    pthread_mutex_destroy(&g_clazz.mutex);
}

