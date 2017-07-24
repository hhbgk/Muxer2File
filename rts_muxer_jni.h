/**
 * Description:
 * Author:created by hhbgk on 17-7-24.
 */
//

#ifndef IJKPLAYER_JL_RTS_MUXER_JNI_H
#define IJKPLAYER_JL_RTS_MUXER_JNI_H
#define TAG "muxer"
#define logi(...)  ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define logw(...)  ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define loge(...)  ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))
int register_rts_muxer(JNIEnv* env);
#endif //IJKPLAYER_JL_RTS_MUXER_JNI_H
