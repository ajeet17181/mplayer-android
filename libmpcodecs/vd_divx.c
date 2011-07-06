/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "mp_msg.h"
#include "vd_internal.h"
#include "libavutil/intreadwrite.h"
#include <omxil/OMX_Core.h>
#include <omxil/OMX_Component.h>
#include <dlfcn.h>
#include <android/log.h>

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



	char psz_name[OMX_MAX_STRINGNAME_SIZE];
        OMX_U32 no_of_roles;
        OMX_U8 **string_of_roles;
	void *dll_handle = 0;
        int index1;
        OMX_BUFFERHEADERTYPE p_header;
	OMX_ERRORTYPE omx_error;
        OMX_HANDLETYPE omx_handle;
        OMX_ERRORTYPE (*pf_init) (void);
        OMX_ERRORTYPE (*pf_deinit) (void);
        OMX_ERRORTYPE (*pf_get_handle) (OMX_HANDLETYPE *, OMX_STRING,OMX_PTR, OMX_CALLBACKTYPE *);
        OMX_ERRORTYPE (*pf_free_handle) (OMX_HANDLETYPE);
        OMX_ERRORTYPE (*pf_component_enum)(OMX_STRING, OMX_U32, OMX_U32);
        OMX_ERRORTYPE (*pf_get_roles_of_component)(OMX_STRING, OMX_U32 *, OMX_U8 **);
        OMX_PARAM_PORTDEFINITIONTYPE input_port_def;
        OMX_PARAM_PORTDEFINITIONTYPE output_port_def;
	OMX_BUFFERHEADERTYPE *outBufferParseVideo[2];
	OMX_BUFFERHEADERTYPE *inBufferVideoDec[2], *outBufferVideoDec[2];
        OMX_PARAM_COMPONENTROLETYPE role;
        OMX_PARAM_PORTDEFINITIONTYPE definition;
        OMX_PORT_PARAM_TYPE param;
        static OMX_CALLBACKTYPE callbacks =
        { OmxEventHandler, OmxEmptyBufferDone, OmxFillBufferDone };



static vd_info_t info =
{
	"divx video decoder",
	"divx",
	"ajeet vijayvergiya",
	"ajeet.vijay@gmail.com",
	"divx decoding"
};

#undef LIBVD_EXTERN
#define LIBVD_EXTERN(x) vd_functions_t mpcodecs_vd_##x = {\
        &info,\
        init,\
        uninit,\
        control,\
        decode\
};

LIBVD_EXTERN(divx)

// to set/get/query special features/parameters
static int control(sh_video_t *sh,int cmd,void* arg,...){
    return CONTROL_UNKNOWN;
}

// init driver
static int init(sh_video_t *sh){
int i;   

uint8_t *extradata = (uint8_t *)(sh->bih + 1);
    int extradata_size = sh->bih->biSize - sizeof(*sh->bih);
	unsigned char *p = extradata;   
for (i=0;i<extradata_size;i++)
  mp_msg(MSGT_VO,MSGL_ERR,"[%x]",extradata[i]);


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
        return 0;
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
                for (index1 = 0; index1 < no_of_roles; index1++)
                {
                        *(string_of_roles + index1) = malloc(no_of_roles * OMX_MAX_STRINGNAME_SIZE);
                }
                omx_error = pf_get_roles_of_component(psz_name, &no_of_roles, string_of_roles);
                if (omx_error == OMX_ErrorNone && string_of_roles!=NULL) {
                for (index1 = 0; index1 < no_of_roles; index1++) {
                __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","The role %i for the component:  %s",(index1+1),*(string_of_roles+index1));
                }
                }
                else    {
                __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error getting role");
                }
        }
        }
        i++;
        }
        omx_error=pf_get_handle( &omx_handle, "OMX.qcom.video.decoder.avc\0",NULL,&callbacks);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Get Handle %s",ErrorToString(omx_error));
	if (omx_handle){
	input_port_def.nPortIndex = 0;
	 omx_error = OMX_GetParameter(omx_handle,
                               OMX_IndexParamPortDefinition,
                               &input_port_def);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","GetPortDefination %s",ErrorToString(omx_error));


        input_port_def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;     
	input_port_def.format.video.nFrameWidth  = sh->disp_w;
        input_port_def.format.video.nFrameHeight = sh->disp_h;
        omx_error = OMX_SetParameter(omx_handle,
                               OMX_IndexParamPortDefinition,
                               &input_port_def);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","SetParameter %s",ErrorToString(omx_error));
	
	omx_error = OMX_SendCommand(omx_handle, OMX_CommandPortEnable, 1, NULL);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","PortEnable %s",ErrorToString(omx_error));
	
	outBufferParseVideo[0] = outBufferParseVideo[1] = NULL;
	omx_error = OMX_SendCommand( omx_handle, OMX_CommandStateSet, OMX_StateIdle, 0 );
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","StateIdle %s",ErrorToString(omx_error));
	sleep(1);
	omx_error = OMX_SendCommand( omx_handle, OMX_CommandStateSet, OMX_StateLoaded, 0 );
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","StateLoaded %s",ErrorToString(omx_error));

	omx_error = OMX_AllocateBuffer(omx_handle, &outBufferParseVideo[0], 0, NULL,16384);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer %s",ErrorToString(omx_error));
	
	omx_error = OMX_AllocateBuffer(omx_handle, &outBufferParseVideo[1], 0, NULL,16384);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer %s",ErrorToString(omx_error));
/*	omx_error = OMX_UseBuffer(omx_handle, &inBufferVideoDec[0], 0, NULL, 16384, outBufferParseVideo[0]->pBuffer);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","UseBuffer 1%s",ErrorToString(omx_error));
	omx_error = OMX_UseBuffer(omx_handle, &inBufferVideoDec[1], 0, NULL, 16384, outBufferParseVideo[1]->pBuffer);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","UseBuffer 2%s",ErrorToString(omx_error));*/
	omx_error = OMX_AllocateBuffer(omx_handle, &outBufferVideoDec[0], 1, NULL, 16384);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer Video 1%s",ErrorToString(omx_error));
	omx_error = OMX_AllocateBuffer(omx_handle, &outBufferVideoDec[1], 1, NULL, 16384);
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer Video 2%s",ErrorToString(omx_error));
	omx_error = OMX_SendCommand(omx_handle, OMX_CommandStateSet,OMX_StateExecuting, 0);
	sleep(1);
	outBufferParseVideo[0]->nFilledLen = extradata_size;
	memcpy(outBufferParseVideo[0]->pBuffer, extradata, extradata_size);
	omx_error=OMX_EmptyThisBuffer(omx_handle, outBufferParseVideo[0]);
        if (omx_error != OMX_ErrorNone) {
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error %s",ErrorToString(omx_error));
        }
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error %s",ErrorToString(omx_error));









        }
	}
        }
        else
        __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Unable to open OMX dll");
        

  return 1;
}

// uninit driver
static void uninit(sh_video_t *sh){
        pf_free_handle( omx_handle );
        pf_deinit();
        dlclose(dll_handle);
}
static int inited=0;
// decode a frame
static mp_image_t* decode(sh_video_t *sh,void* data,int len,int flags){
mp_image_t *mpi;

 if(!data || len <= 0 ||  flags&2)
{ 
mp_msg(MSGT_VO,MSGL_ERR,"No Len\n");
  return NULL;
}
	if (len<=16384){
	memcpy(outBufferParseVideo[0]->pBuffer, data, len );
	outBufferParseVideo[0]->nFilledLen=len;
	OMX_EmptyThisBuffer(omx_handle, outBufferParseVideo[0]);
	}
 
mpi = mpcodecs_get_image(sh,MP_IMGTYPE_EXPORT,MP_IMGFLAG_ACCEPT_STRIDE,sh->disp_w,sh->disp_h);
   if(!mpi){
   mp_msg(MSGT_VO,MSGL_ERR,"No MPI\n");
	 return NULL;
   }	

	return NULL;
}

