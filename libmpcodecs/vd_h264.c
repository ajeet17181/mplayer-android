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
#include <stdbool.h>
#include <libavformat/avformat.h>

static int nal_len_size;
static const uint8_t nalu_header[4] = {0, 0, 0, 1};

static int extradata_convert(AVCodecContext *c)
{
  const uint8_t *p;
  int total_size = 0;
  uint8_t *out = NULL;
  uint8_t n;

  if (c->extradata_size == 0 || c->extradata == NULL) {
    return 0;
  }

  if (*(uint8_t *)c->extradata != 1) {
    return 0;
  }

  p = c->extradata + 4;
  nal_len_size = (p[0] & 0x03) + 1;
  n = p[1] & 0x1F;
  p += 2;
  while (n--) {
    uint16_t nal_size;

    nal_size = p[0] << 8 | p[1];
    total_size += nal_size + sizeof(nalu_header);
    if (p + 2 + nal_size > c->extradata + c->extradata_size) {
      av_free(out);

      return -1;
    }
    out = av_realloc(out, total_size);
    if (!out) {
      return -1;
    }
    memcpy(out + total_size - nal_size - sizeof(nalu_header), nalu_header, sizeof(nalu_header));
    memcpy(out + total_size - nal_size, p + 2, nal_size);
    p += 2 + nal_size;

    if (n == 0 && p + 2 < c->extradata + c->extradata_size) {
      n = *p++;
    }
  }
  av_free(c->extradata);
  c->extradata = out;
  c->extradata_size = total_size;

  return total_size;
}
static const char *ErrorToString(OMX_ERRORTYPE error)
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



static char psz_name[OMX_MAX_STRINGNAME_SIZE];
static OMX_U32 no_of_roles;
static OMX_U8 **string_of_roles;
static void *dll_handle = 0;
static int index1;
static OMX_BUFFERHEADERTYPE p_header;
static OMX_ERRORTYPE omx_error;
static OMX_HANDLETYPE omx_handle;
static OMX_ERRORTYPE (*pf_init) (void);
static OMX_ERRORTYPE (*pf_deinit) (void);
static OMX_ERRORTYPE (*pf_get_handle) (OMX_HANDLETYPE *, OMX_STRING, OMX_PTR,
		OMX_CALLBACKTYPE *);
static OMX_ERRORTYPE (*pf_free_handle) (OMX_HANDLETYPE);
static OMX_ERRORTYPE (*pf_component_enum) (OMX_STRING, OMX_U32, OMX_U32);
static OMX_ERRORTYPE (*pf_get_roles_of_component) (OMX_STRING, OMX_U32 *,
		OMX_U8 **);
static OMX_PARAM_PORTDEFINITIONTYPE input_port_def;
static OMX_PARAM_PORTDEFINITIONTYPE output_port_def;
static OMX_BUFFERHEADERTYPE *outBufferParseVideo[2];
static OMX_BUFFERHEADERTYPE *inBufferVideoDec[2], *outBufferVideoDec[2];
static OMX_PARAM_COMPONENTROLETYPE role;
static OMX_PARAM_PORTDEFINITIONTYPE definition;
static OMX_PORT_PARAM_TYPE param;
static OMX_CALLBACKTYPE callbacks =
{ OmxEventHandler, OmxEmptyBufferDone, OmxFillBufferDone };



static vd_info_t info =
{
	"H264 video decoder",
	"h264",
	"ajeet vijayvergiya",
	"ajeet.vijay@gmail.com",
	"H264 decoding"
};

#undef LIBVD_EXTERN
#define LIBVD_EXTERN(x) vd_functions_t mpcodecs_vd_##x = {\
	&info,\
	init,\
	uninit,\
	control,\
	decode\
};

LIBVD_EXTERN(h264)

	// to set/get/query special features/parameters
	static int control(sh_video_t *sh,int cmd,void* arg,...){
		return CONTROL_UNKNOWN;
	}

// init driver
static int init(sh_video_t *sh){
	AVCodecContext *c;
	int i;   
	int i_sps, i_pps;
	bool b_dummy;
	uint8_t *extradata = (uint8_t *)(sh->bih + 1);
	int extradata_size = sh->bih->biSize - sizeof(*sh->bih);
	c=malloc(sizeof(AVCodecContext));
	c->extradata=malloc(extradata_size);
	c->extradata_size=extradata_size;
	extradata_convert(c);


	//for (i=0;i<extradata_size;i++)
	//  mp_msg(MSGT_VO,MSGL_ERR,"[%x]",extradata[i]);


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
				omx_error = OMX_GetParameter(omx_handle,
						OMX_IndexParamPortDefinition,
						&input_port_def);
				__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","GetPortDefination %s",ErrorToString(omx_error));


				input_port_def.nPortIndex = 0;
				input_port_def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;     	input_port_def.bEnabled = OMX_TRUE;
				input_port_def.bPopulated = OMX_FALSE;
				input_port_def.eDir = OMX_DirInput;
				input_port_def.eDomain = OMX_PortDomainVideo;
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

				omx_error = OMX_AllocateBuffer(omx_handle, &outBufferParseVideo[0], 0, NULL,65535);
				__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer %s",ErrorToString(omx_error));

				/*	omx_error = OMX_AllocateBuffer(omx_handle, &outBufferParseVideo[1], 0, NULL,16384);
					__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer %s",ErrorToString(omx_error));
					omx_error = OMX_UseBuffer(omx_handle, &inBufferVideoDec[0], 0, NULL, 16384, outBufferParseVideo[0]->pBuffer);
					__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","UseBuffer 1%s",ErrorToString(omx_error));
					omx_error = OMX_UseBuffer(omx_handle, &inBufferVideoDec[1], 0, NULL, 16384, outBufferParseVideo[1]->pBuffer);
					__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","UseBuffer 2%s",ErrorToString(omx_error));*/
				omx_error = OMX_AllocateBuffer(omx_handle, &outBufferVideoDec[0], 1, NULL, 16384);
				__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer Video 1%s",ErrorToString(omx_error));
				/*	omx_error = OMX_AllocateBuffer(omx_handle, &outBufferVideoDec[1], 1, NULL, 16384);
					__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","AllocateBuffer Video 2%s",ErrorToString(omx_error));*/
				omx_error = OMX_SendCommand(omx_handle, OMX_CommandStateSet,OMX_StateExecuting, 0);
				sleep(1);
				outBufferParseVideo[0]->nFilledLen = c->extradata_size;
				outBufferParseVideo[0]->nFlags = OMX_BUFFERFLAG_CODECCONFIG;	
				outBufferParseVideo[0]->nOffset = 0;
				memcpy(outBufferParseVideo[0]->pBuffer, c->extradata, c->extradata_size);
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
int i=0;

// uninit driver
static void uninit(sh_video_t *sh){
	pf_free_handle( omx_handle );
	pf_deinit();
	dlclose(dll_handle);
	__android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Frame=%d",i);
}
static int inited=0;


static int stream_convert(AVPacket *pkt)
{
  uint8_t *out = NULL;
  const uint8_t *p = pkt->data;
  int size = 0;

  if (nal_len_size == 0) {
    return 0;
  }

  while(p + 4 < pkt->data + pkt->size) {
    uint32_t nal_len;

    if (nal_len_size != 4) {
      return -1;
    }
    nal_len = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];

    out = av_realloc(out, size + nal_len + sizeof(nalu_header));
    memcpy(out + size, nalu_header, sizeof(nalu_header));
    if (p + nal_len_size + nal_len > pkt->data + pkt->size) {
      fprintf(stderr, "NAL Size is broken: %d > %d\n", p - pkt->data + nal_len_size + nal_len, pkt->size);
      nal_len = pkt->data + pkt->size - (p + nal_len_size);
    }
    memcpy(out + size + sizeof(nalu_header), p + nal_len_size, nal_len);
    size += nal_len + sizeof(nalu_header);
    p    += nal_len + nal_len_size;
  }
  if (p != pkt->data + pkt->size) {
    fprintf(stderr, "Strange: %d != %d\n", p - pkt->data, pkt->size);
  }
  av_free(pkt->data);
  pkt->data = out;
  pkt->size = size;

  return 0;
}

// decode a frame
static mp_image_t* decode(sh_video_t *sh,void* data,int len,int flags){
	AVPacket *pkt;
	mp_image_t *mpi;
	i++;
	//       __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","len=%d",len);

	if(!data || len <= 0 )
	{ 
		//mp_msg(MSGT_VO,MSGL_ERR,"No Len\n");
		return NULL;
	}
	
	pkt=malloc(sizeof(AVPacket));
	pkt->data=malloc(len);
	pkt->size=len;
	stream_convert(pkt);
	memcpy(outBufferParseVideo[0]->pBuffer, pkt->data, pkt->size );
	outBufferParseVideo[0]->nFilledLen=pkt->size;
	outBufferParseVideo[0]->nTimeStamp=sh->timer*90000.0;
	outBufferParseVideo[0]->nOffset = 0;
	outBufferParseVideo[0]->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;
	OMX_EmptyThisBuffer(omx_handle, outBufferParseVideo[0]);

	free(pkt);
	/* 
	   mpi = mpcodecs_get_image(sh,MP_IMGTYPE_EXPORT,MP_IMGFLAG_ACCEPT_STRIDE,sh->disp_w,sh->disp_h);
	   if(!mpi){
	//  mp_msg(MSGT_VO,MSGL_ERR,"No MPI\n");
	return NULL;
	}	
	*/
	return NULL;
}

