#include <dlfcn.h>
#include <android/log.h>
#include <jni.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static JavaVM *jniVM=NULL;
static JNIEnv* JavaEnv = NULL;



#define OMX_INIT_COMMON(a) \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = 1; \
  (a).nVersion.s.nVersionMinor = 1; \
  (a).nVersion.s.nRevision = 1; \
  (a).nVersion.s.nStep = 0

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  OMX_INIT_COMMON(a)


const char *ErrorToString(OMX_ERRORTYPE error)
{
    static const char *psz_names[] = {
        "OMX_ErrorInsufficientResources", "OMX_ErrorUndefined",
        "OMX_ErrorInvalidComponentName", "OMX_ErrorComponentNotFound",
        "OMX_ErrorInvalidComponent", "OMX_ErrorBadParameter",
        "OMX_ErrorNotImplemented", "OMX_ErrorUnderflow",
        "OMX_ErrorOverflow", "OMX_ErrorHardware", "OMX_ErrorInvalidState",
        "OMX_ErrorStreamCorrupt", "OMX_ErrorPortsNotCompatible",
        "OMX_ErrorResourcesLost", "OMX_ErrorNoMore", "OMX_ErrorVersionMismatch",
        "OMX_ErrorNotReady", "OMX_ErrorTimeout", "OMX_ErrorSameState",
        "OMX_ErrorResourcesPreempted", "OMX_ErrorPortUnresponsiveDuringAllocation",
        "OMX_ErrorPortUnresponsiveDuringDeallocation",
        "OMX_ErrorPortUnresponsiveDuringStop", "OMX_ErrorIncorrectStateTransition",
        "OMX_ErrorIncorrectStateOperation", "OMX_ErrorUnsupportedSetting",
        "OMX_ErrorUnsupportedIndex", "OMX_ErrorBadPortIndex",
        "OMX_ErrorPortUnpopulated", "OMX_ErrorComponentSuspended",
        "OMX_ErrorDynamicResourcesUnavailable", "OMX_ErrorMbErrorsInFrame",
        "OMX_ErrorFormatNotDetected", "OMX_ErrorContentPipeOpenFailed",
        "OMX_ErrorContentPipeCreationFailed", "OMX_ErrorSeperateTablesUsed",
        "OMX_ErrorTunnelingUnsupported",
        "OMX_Error unkown"
    };

    if(error == OMX_ErrorNone) return "OMX_ErrorNone";

    error -= OMX_ErrorInsufficientResources;

    if((unsigned int)error > sizeof(psz_names)/sizeof(char*)-1)
        error = (OMX_STATETYPE)(sizeof(psz_names)/sizeof(char*)-1);
    return psz_names[error];
}

static OMX_ERRORTYPE OmxEventHandler( OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data, OMX_EVENTTYPE event, OMX_U32 data_1,
    OMX_U32 data_2, OMX_PTR event_data )
{
return OMX_ErrorNone;
}


static OMX_ERRORTYPE OmxEmptyBufferDone( OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data, OMX_BUFFERHEADERTYPE *omx_header )
{
return OMX_ErrorNone;
}

static OMX_ERRORTYPE OmxFillBufferDone( OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data, OMX_BUFFERHEADERTYPE *omx_header )
{
return OMX_ErrorNone;
}

JNIEXPORT void JNICALL Java_com_test_omx_test_init(JNIEnv *env, jobject  obj){
        (*jniVM)->AttachCurrentThread(jniVM, &env, NULL);
	char psz_name[OMX_MAX_STRINGNAME_SIZE];
	OMX_U32 no_of_roles;
	OMX_U8 **string_of_roles;	
	int index;
	OMX_ERRORTYPE omx_error;
	OMX_HANDLETYPE omx_handle;
	OMX_ERRORTYPE (*pf_init) (void);
    	OMX_ERRORTYPE (*pf_deinit) (void);
    	OMX_ERRORTYPE (*pf_get_handle) (OMX_HANDLETYPE *, OMX_STRING,OMX_PTR, OMX_CALLBACKTYPE *);
    	OMX_ERRORTYPE (*pf_free_handle) (OMX_HANDLETYPE);
    	OMX_ERRORTYPE (*pf_component_enum)(OMX_STRING, OMX_U32, OMX_U32);
    	OMX_ERRORTYPE (*pf_get_roles_of_component)(OMX_STRING, OMX_U32 *, OMX_U8 **);
	OMX_PARAM_PORTDEFINITIONTYPE* input_port_def;
    	OMX_PARAM_PORTDEFINITIONTYPE* output_port_def;
	
	OMX_PARAM_COMPONENTROLETYPE role;
    	OMX_PARAM_PORTDEFINITIONTYPE definition;
	OMX_PORT_PARAM_TYPE param;
	static OMX_CALLBACKTYPE callbacks =
        { OmxEventHandler, OmxEmptyBufferDone, OmxFillBufferDone };

	
	void *dll_handle = 0; 
	int i=0;
	
	dll_handle=dlopen("libOmxCore.so",RTLD_NOW|RTLD_GLOBAL);
	if (dll_handle){
	pf_init = dlsym( dll_handle, "OMX_Init" );
    	pf_deinit = dlsym( dll_handle, "OMX_Deinit" );
    	pf_get_handle = dlsym( dll_handle, "OMX_GetHandle" );
    	pf_free_handle = dlsym( dll_handle, "OMX_FreeHandle" );
    	pf_component_enum = dlsym( dll_handle, "OMX_ComponentNameEnum" );
    	pf_get_roles_of_component = dlsym( dll_handle, "OMX_GetRolesOfComponent" );
    	if( !pf_init || !pf_deinit || !pf_get_handle || !pf_free_handle ||
        !pf_component_enum || !pf_get_roles_of_component )
    	{
 	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error in OMX");        
    	}
	else
	{
	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Success in OMX"); 
	omx_error = pf_init();
    	if(omx_error != OMX_ErrorNone)
    	{
	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","OMX Init Failed");
	return;
	}
	while (1){
	omx_error=pf_component_enum(psz_name, OMX_MAX_STRINGNAME_SIZE, i);
	if(omx_error != OMX_ErrorNone) break;
	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Component %d,%s",i,psz_name);
	omx_error = pf_get_roles_of_component(psz_name, &no_of_roles, NULL);
	if (omx_error != OMX_ErrorNone) {
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","No Role for Component %s",psz_name);
	}
	else {
	if(no_of_roles == 0) {
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","No Role for Component -2  %s",psz_name);
	}
	else
	{
		string_of_roles = malloc(no_of_roles * sizeof(OMX_STRING));
		for (index = 0; index < no_of_roles; index++) 
		{
			*(string_of_roles + index) = malloc(no_of_roles * OMX_MAX_STRINGNAME_SIZE);
   		}
		omx_error = pf_get_roles_of_component(psz_name, &no_of_roles, string_of_roles);	
		if (omx_error == OMX_ErrorNone && string_of_roles!=NULL) {
		for (index = 0; index < no_of_roles; index++) {
        	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","The role %i for the component:  %s",(index+1),*(string_of_roles+index));
		}
		}
		else	{
        	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error getting role");
		}	
	}
	}
	i++;
	}
	omx_error=pf_get_handle( &omx_handle, "OMX.qcom.video.decoder.avc\0",NULL,&callbacks);
	if(omx_error != OMX_ErrorNone)
       	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error getting handle %s",ErrorToString(omx_error));
       	else	{
	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Got handle");
	input_port_def=malloc(sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

	input_port_def->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC; 	      input_port_def->format.video.nFrameWidth  = 720;
  	input_port_def->format.video.nFrameHeight = 480;
  	input_port_def->nBufferSize=256*1024;
	OMX_ERRORTYPE omxresult = OMX_ErrorNone;
  	omx_error = OMX_SetParameter(omx_handle,
                               OMX_IndexParamPortDefinition,
                               input_port_def);
  	if (omx_error != OMX_ErrorNone) {
    	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error %s",ErrorToString(omx_error));
	}
	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error %s",ErrorToString(omx_error));
	










	
	
	}
	pf_free_handle( omx_handle );

 
	







	
	}
	pf_deinit();
	dlclose(dll_handle);
	}
	else
	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Unable to open OMX dll"); 
	}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
        jniVM = vm;
        (*jniVM)->AttachCurrentThread(jniVM, &JavaEnv, NULL);
        return JNI_VERSION_1_2;
};


