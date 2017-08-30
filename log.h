#ifndef __LOG_H
#define __LOG_H

#ifndef OS_UBUNTU
#include <android/log.h>
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "[ups]"
#ifndef OS_UBUNTU
#define LOGI(...) ((void)__android_log_print( ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print( ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print( ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#else
#define LOGI(...) ((void)printf( LOG_TAG __VA_ARGS__))
#define LOGD(...) ((void)printf( LOG_TAG __VA_ARGS__))
#define LOGW(...) ((void)printf( LOG_TAG __VA_ARGS__))
#define LOGE(...) ((void)printf( LOG_TAG __VA_ARGS__))
#endif

#endif
