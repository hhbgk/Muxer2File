/**
 * Description:
 * Author:created by bob on 17-7-24.
 */
#include <jni.h>
#include <stdio.h>
#include "rts_muxer.h"

static JavaVM *gJVM = NULL;
static jobject gObj = NULL;

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#define CLASS_STREAM "com/hhbgk/media/codec/StreamRecord"

static rts_muxer_t gRTS_muxer;

static jboolean jni_init(JNIEnv *env, jobject thiz)
{
    loge("%s", __func__);

    (*env)->GetJavaVM(env, &gJVM);
    gObj = (*env)->NewGlobalRef(env, thiz);
    jclass clazz = (*env)->GetObjectClass(env, thiz);
    if(clazz == NULL)
    {
        (*env)->ThrowNew(env, "java/lang/NullPointerException", "Unable to find exception class");
    }

    return JNI_TRUE;
}
static jboolean jni_rec_create (JNIEnv *env, jobject thiz,
                  jstring jpath) {
    if (jpath == NULL) return JNI_FALSE;
    const uint8_t *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    const int len = (*env)->GetStringUTFLength(env, jpath);
    int ret = -1;
    rts_muxer_t *client = &gRTS_muxer;
    memset(client, 0, sizeof(rts_muxer_t));
    if (path) {
        ret = rts_muxer_create(client, path);
    }
    return ret == 0 ? JNI_TRUE : JNI_FALSE;
}
static jboolean jni_rec_write(JNIEnv *env, jobject thiz,
                  jint jtype, jbyteArray jbyteArray1, jint jsize) {
    rts_muxer_t *client = &gRTS_muxer;
    uint8_t *data = (*env)->GetByteArrayElements(env, jbyteArray1, NULL);
    int ret = rts_muxer_write(client, jtype, data, jsize);
    if (data)
        (*env)->ReleaseByteArrayElements(env, jbyteArray1, (jbyte *) data, NULL);
    return ret == 0 ? JNI_TRUE : JNI_FALSE;
}
static jboolean jni_rec_close(JNIEnv *env, jobject thiz) {
    rts_muxer_t *client = &gRTS_muxer;
    int ret = rts_muxer_close(client);
    return ret == 0 ? JNI_TRUE : JNI_FALSE;
}
static JNINativeMethod g_methods[] =
{
        {"nativeInit", "()Z", (void *) jni_init},
        {"nativeCreateClient", "(Ljava/lang/String;)Z", (void*) jni_rec_create},
        {"nativeCloseClient", "()Z", (void*) jni_rec_close},
        {"nativeWrite", "(I[BI)Z", (void*) jni_rec_write},
};

int register_rts_muxer(JNIEnv* env)
{
    jclass klass = (*env)->FindClass(env, CLASS_STREAM);
    if (klass == NULL) {
        return JNI_ERR;
    }
    (*env)->RegisterNatives(env, klass, g_methods, NELEM(g_methods));

    return JNI_VERSION_1_4;
}
