#ifndef __LOG_H
#define __LOG_H

#ifndef OS_UBUNTU
#include <android/log.h>
#endif

#ifndef GLOBAL_TAG
#define GLOBAL_TAG "[ups]"
#endif

#ifndef LOG_TAG
#define LOG_TAG 
#endif

#define FINAL_LOG_TAG  GLOBAL_TAG LOG_TAG

#ifndef OS_UBUNTU
#define LOGI(...) ((void)__android_log_print( ANDROID_LOG_INFO, FINAL_LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print( ANDROID_LOG_DEBUG, FINAL_LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print( ANDROID_LOG_WARN, FINAL_LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, FINAL_LOG_TAG, __VA_ARGS__))
#else
#define LOGI(...) ((void)printf( FINAL_LOG_TAG __VA_ARGS__))
#define LOGD(...) ((void)printf( FINAL_LOG_TAG __VA_ARGS__))
#define LOGW(...) ((void)printf( FINAL_LOG_TAG __VA_ARGS__))
#define LOGE(...) ((void)printf( FINAL_LOG_TAG __VA_ARGS__))
#endif

#endif
