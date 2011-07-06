#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include "metadata.h"
#include <pthread.h>
//#include <tag_c.h>

static JNIEnv* JavaEnv = NULL;
static jclass JavaRendererClass = NULL;
static jclass JavaMplayerClass = NULL;
static jobject JavaRenderer = NULL;
static jobject JavaMplayer = NULL;
static jmethodID jnativeConfig = NULL;
static jmethodID jnativeAudio = NULL;
static jmethodID jnativeDeconfig = NULL;
static jmethodID jupdate = NULL;
static jmethodID jupdate1 = NULL;
static jmethodID jupdate_main = NULL;
static jmethodID jupdate_main_string = NULL;
static jmethodID jstop = NULL;
static jobject video;
pthread_t native_thread;
const jbyte *str;
char* command = NULL;
static JavaVM *jniVM=NULL;
int arm_vfp=0;
int arm_neon=0;
static int w1,h1;
jobject temp;
int i,x;

void update_main_string(uint8_t *name){
jstring jstr;
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
jstr=(*JavaEnv)->NewStringUTF(JavaEnv,name);
(*JavaEnv)->CallVoidMethod( JavaEnv,JavaMplayer, jupdate_main_string,jstr);
(*JavaEnv)->DeleteLocalRef(JavaEnv,jstr);
}

void main_init(uint8_t *src[]){
	(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
        temp=(*JavaEnv)->NewDirectByteBuffer(JavaEnv,src[0],w1*h1*2);
        video=(jobject)(*JavaEnv)->NewGlobalRef(JavaEnv,temp);
        (*JavaEnv)->CallVoidMethod( JavaEnv, JavaRenderer, jupdate,video);
}

void main_flip(){
	(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
	(*JavaEnv)->CallVoidMethod( JavaEnv, JavaRenderer, jupdate1);
}


void main_stop(int code){
	(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
	(*JavaEnv)->CallVoidMethod( JavaEnv, JavaMplayer, jstop,code);
}

void main_ao_init(){
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
(*JavaEnv)->CallVoidMethod( JavaEnv, JavaMplayer, jnativeAudio);
}

void update(int percent,int current,int total,int pause){
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
(*JavaEnv)->CallVoidMethod( JavaEnv, JavaMplayer, jupdate_main,percent,current,total,pause);
}

JNIEXPORT void JNICALL Java_com_vnd_mplayer_PlasmaView_init(JNIEnv *env, jobject  obj,jint delay)
{
	(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
	 JavaEnv = env;
 	 JavaRenderer = obj;
	 JavaRendererClass = (*JavaEnv)->GetObjectClass(JavaEnv, obj);
	 jnativeConfig = (*JavaEnv)->GetMethodID(JavaEnv, JavaRendererClass, "jnativeConfig", "(II)I");
	 jnativeDeconfig = (*JavaEnv)->GetMethodID(JavaEnv, JavaRendererClass, "jnativeDeconfig", "()V");
	 jupdate = (*JavaEnv)->GetMethodID(JavaEnv, JavaRendererClass, "update", "(Ljava/nio/ByteBuffer;)V");
	 jupdate1 = (*JavaEnv)->GetMethodID(JavaEnv, JavaRendererClass, "update1", "()V");
}


JNIEXPORT void JNICALL Java_com_vnd_mplayer_MPlayer_nativeExit(JNIEnv *env, jobject  obj)
{
	//__android_log_print(ANDROID_LOG_ERROR,"MPlayer","stop called");
	run_command_android("stop");
}

JNIEXPORT void JNICALL Java_com_vnd_mplayer_MPlayer_play(JNIEnv *env, jobject  obj,jstring fname,int cpuflags){
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
 JavaMplayer = obj;
 JavaMplayerClass = (*JavaEnv)->GetObjectClass(JavaEnv, obj);
 jnativeAudio = (*JavaEnv)->GetMethodID(JavaEnv, JavaMplayerClass, "jnativeAudio", "()V");
 jstop = (*JavaEnv)->GetMethodID(JavaEnv, JavaMplayerClass, "stop", "(I)V");
 jupdate_main = (*JavaEnv)->GetMethodID(JavaEnv, JavaMplayerClass, "update_main", "(IIII)V");
 jupdate_main_string = (*JavaEnv)->GetMethodID(JavaEnv, JavaMplayerClass, "update_main_string", "(Ljava/lang/String;)V");
if (cpuflags==1)
arm_vfp=1;
if (cpuflags==2)
arm_neon=1;
if (cpuflags==3){
arm_vfp=1;arm_neon=1;}
str = (*JavaEnv)->GetStringUTFChars(JavaEnv, fname, NULL);
char *temp;
temp=malloc(255);
if ((strstr(str,".m3u")!=NULL) || (strstr(str,".pls")!=NULL))
strcpy(temp,"loadlist \"");
else
strcpy(temp,"loadfile \"");
strcat(temp,str);
strcat(temp,"\"");
run_command_android(temp);
free(temp);
}


JNIEXPORT void JNICALL Java_com_vnd_mplayer_MPlayer_nativeCommand(JNIEnv *env, jobject  obj,jint command,jint x,jint y)
{

switch (command){
	case 1:	//Pause
		if (x==0 && isPaused()==1)
		run_command_android("pause");
		if (x==1 && isPaused()==0)
		run_command_android("pause");
		if (x==2)
		run_command_android("pause");
		break;
	case 2:  //Seek
		command=(void *)malloc(20);
		sprintf((void *)command,"seek %d 1",x);
		run_command_android(command);
		free((void *)command);
		break;
	case 3:
		if (x==1)
		 run_command_android("osd 3");
		else
		 run_command_android("osd 0");
		break;
	case 4:
		if (x==1)
		 run_command_android("pt_step 1 1");
		else
		run_command_android("pt_step -1 1");
		break;
		
}
}

JNIEXPORT jint JNICALL Java_com_vnd_mplayer_MPlayer_ispaused(JNIEnv *env, jobject  obj){
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
return isPaused();
}

int nativeConfig(int w,int h){
//mp_msg(MSGT_CPLAYER,MSGL_ERR,"nativeConfig called %d %d",w,h);
w1=w;
h1=h;
i=0;
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
return((*JavaEnv)->CallIntMethod( JavaEnv, JavaRenderer, jnativeConfig,w,h ));
}


void nativeDeconfig(){
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
(*JavaEnv)->DeleteGlobalRef(JavaEnv,video);
(*JavaEnv)->CallVoidMethod (JavaEnv, JavaRenderer, jnativeDeconfig);
}

JNIEXPORT int JNICALL Java_com_vnd_mplayer_MPlayer_nativeTouch(JNIEnv *env, jobject thiz,jfloat x,jfloat y) {

if (x==0)
run_command_android("seek +1");
else
run_command_android("seek -1");

}

//JNIEXPORT int JNICALL Java_com_vnd_mplayer_MPlayer_main(JNIEnv *env, jclass cls) {
void mainp(void){
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
int argc=10;char *argv[] ={ "mplayer","-idle","-slave","-osdlevel","0","-vf","format=bgr16","-noaspect","-nocorrect-pts","-quiet"};
main(argc,argv);
}


JNIEXPORT int JNICALL Java_com_vnd_mplayer_MPlayer_quit(JNIEnv *env, jclass cls) {
run_command_android("quit");
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
        jniVM = vm;
	(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
	pthread_create(&native_thread,NULL,mainp,NULL);
        return JNI_VERSION_1_2;
};

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
       	 jniVM = vm;
	 run_command_android("quit");
	 pthread_exit(&native_thread);
};

void run_command_android(char *command){
mp_input_queue_cmd(mp_input_parse_cmd(command));
}

JNIEXPORT int JNICALL Java_com_vnd_mplayer_MPlayer_isalbum(JNIEnv *env, jobject  obj,jstring fname,jstring outfile){
(*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
const char *str,*fileout;
str = (*env)->GetStringUTFChars(env,fname, NULL);
fileout = (*env)->GetStringUTFChars(env,outfile, NULL);
if (isalbum(str,fileout)){
(*env)->ReleaseStringUTFChars(env, fname, str);
(*env)->ReleaseStringUTFChars(env, outfile, fileout);
return 1;
}
(*env)->ReleaseStringUTFChars(env, fname, str);
(*env)->ReleaseStringUTFChars(env, outfile, fileout);
return 0;
}
int have_neon(){
return arm_neon;
}
