#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
      //  jniVM = vm;
        return JNI_VERSION_1_2;
};

JNIEXPORT void JNICALL Java_com_android_mplayer_Video_load(JNIEnv *env, jobject  obj){
void* dll;
dll=dlopen("/data/data/com.android.mplayer/lib/libmplayer.so",RTLD_GLOBAL);
if (dll)
__android_log_print(ANDROID_LOG_ERROR,"MPlayer","Pass");
else
__android_log_print(ANDROID_LOG_ERROR,"MPlayer","fail");
}
JNIEXPORT void JNICALL Java_com_android_mplayer_Video_unload(JNIEnv *env, jobject  obj){
dlclose("mplayer");
}
