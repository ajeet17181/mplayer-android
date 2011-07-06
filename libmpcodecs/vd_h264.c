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
#define H264 0

#if H264

static inline uint16_t U16_AT( const void * _p )
 {
     const uint8_t * p = (const uint8_t *)_p;
     return ( ((uint16_t)p[0] << 8) | p[1] );
 }

typedef int64_t mtime_t;
//typedef void (*block_free_t) (block_t *);

 struct block_t
 {
     block_t     *p_next;
 
     uint32_t    i_flags;
 
     mtime_t     i_pts;
     mtime_t     i_dts;
     mtime_t     i_length;
 
     unsigned    i_nb_samples; /* Used for audio */
     int         i_rate;
 
     size_t      i_buffer;
     uint8_t     *p_buffer;
 
     /* Rudimentary support for overloading block (de)allocation. */
     //block_free_t pf_release;
 };

typedef struct
{
    int i_nal_type;
    int i_nal_ref_idc;

    int i_frame_type;
    int i_pic_parameter_set_id;
    int i_frame_num;

    int i_field_pic_flag;
    int i_bottom_field_flag;

    int i_idr_pic_id;

    int i_pic_order_cnt_lsb;
    int i_delta_pic_order_cnt_bottom;

    int i_delta_pic_order_cnt0;
    int i_delta_pic_order_cnt1;
} slice_t;

#define SPS_MAX (32)
#define PPS_MAX (256)
typedef struct 
{
    /* */
    //packetizer_t packetizer;

    /* */
    bool    b_slice;
    block_t *p_frame;
    bool    b_frame_sps;
    bool    b_frame_pps;

    bool   b_header;
    bool   b_sps;
    bool   b_pps;
    block_t *pp_sps[SPS_MAX];
    block_t *pp_pps[PPS_MAX];
    int    i_recovery_frames;  /* -1 = no recovery */

    /* avcC data */
    int i_avcC_length_size;

    /* Useful values of the Sequence Parameter Set */
    int i_log2_max_frame_num;
    int b_frame_mbs_only;
    int i_pic_order_cnt_type;
    int i_delta_pic_order_always_zero_flag;
    int i_log2_max_pic_order_cnt_lsb;

    /* Value from Picture Parameter Set */
    int i_pic_order_present_flag;

    /* Useful values of the Slice Header */
    slice_t slice;

    /* */
    mtime_t i_frame_pts;
    mtime_t i_frame_dts;

    /* */
    uint32_t i_cc_flags;
    mtime_t i_cc_pts;
    mtime_t i_cc_dts;
   // cc_data_t cc;

    //cc_data_t cc_next;
}decoder_sys_t;

enum nal_unit_type_e
{
    NAL_UNKNOWN = 0,
    NAL_SLICE   = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,
    NAL_PPS         = 8,
    NAL_AU_DELIMITER= 9
    /* ref_idc == 0 for 6,9,10,11,12 */
};

enum nal_priority_e
{
    NAL_PRIORITY_DISPOSABLE = 0,
    NAL_PRIORITY_LOW        = 1,
    NAL_PRIORITY_HIGH       = 2,
    NAL_PRIORITY_HIGHEST    = 3,
};

static inline void block_ChainAppend( block_t **pp_list, block_t *p_block )
 {
     if( *pp_list == NULL )
     {
         *pp_list = p_block;
     }
     else
     {
         block_t *p = *pp_list;
 
         while( p->p_next ) p = p->p_next;
         p->p_next = p_block;
     }
 }
#endif

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
 static        OMX_U32 no_of_roles;
 static        OMX_U8 **string_of_roles;
	static void *dll_handle = 0;
 static        int index1;
       static  OMX_BUFFERHEADERTYPE p_header;
	static OMX_ERRORTYPE omx_error;
  static       OMX_HANDLETYPE omx_handle;
 static        OMX_ERRORTYPE (*pf_init) (void);
 static        OMX_ERRORTYPE (*pf_deinit) (void);
 static        OMX_ERRORTYPE (*pf_get_handle) (OMX_HANDLETYPE *, OMX_STRING,OMX_PTR, OMX_CALLBACKTYPE *);
 static        OMX_ERRORTYPE (*pf_free_handle) (OMX_HANDLETYPE);
 static        OMX_ERRORTYPE (*pf_component_enum)(OMX_STRING, OMX_U32, OMX_U32);
 static        OMX_ERRORTYPE (*pf_get_roles_of_component)(OMX_STRING, OMX_U32 *, OMX_U8 **);
 static        OMX_PARAM_PORTDEFINITIONTYPE input_port_def;
 static        OMX_PARAM_PORTDEFINITIONTYPE output_port_def;
	static OMX_BUFFERHEADERTYPE *outBufferParseVideo[2];
	static OMX_BUFFERHEADERTYPE *inBufferVideoDec[2], *outBufferVideoDec[2];
    static     OMX_PARAM_COMPONENTROLETYPE role;
 static        OMX_PARAM_PORTDEFINITIONTYPE definition;
 static        OMX_PORT_PARAM_TYPE param;
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
#if H264
static block_t *CreateAnnexbNAL( const uint8_t *p, int i_size )
{
    block_t *p_nal;
    p_nal=malloc(sizeof(block_t));
    p_nal->p_buffer = malloc(4 + i_size );
    if( !p_nal ) return NULL;

    /* Add start code */
    p_nal->p_buffer[0] = 0x00;
    p_nal->p_buffer[1] = 0x00;
    p_nal->p_buffer[2] = 0x00;
    p_nal->p_buffer[3] = 0x01;

    /* Copy nalu */
    memcpy( &p_nal->p_buffer[4], p, i_size );
    return p_nal;
}

static block_t *ParseNALBlock( bool *pb_used_ts, block_t *p_frag )
{
    decoder_sys_t *p_sys;
    block_t *p_pic = NULL;

    const int i_nal_ref_idc = (p_frag->p_buffer[4] >> 5)&0x03;
    const int i_nal_type = p_frag->p_buffer[4]&0x1f;
    const mtime_t i_frag_dts = p_frag->i_dts;
    const mtime_t i_frag_pts = p_frag->i_pts;

    if( p_sys->b_slice && ( !p_sys->b_sps || !p_sys->b_pps ) )
    {
        block_ChainRelease( p_sys->p_frame );
        msg_Warn( p_dec, "waiting for SPS/PPS" );

        /* Reset context */
        p_sys->slice.i_frame_type = 0;
        p_sys->p_frame = NULL;
        p_sys->b_frame_sps = false;
        p_sys->b_frame_pps = false;
        p_sys->b_slice = false;
        cc_Flush( &p_sys->cc_next );
    }

    if( ( !p_sys->b_sps || !p_sys->b_pps ) &&
        i_nal_type >= NAL_SLICE && i_nal_type <= NAL_SLICE_IDR )
    {
        p_sys->b_slice = true;
        /* Fragment will be discarded later on */
    }
    else if( i_nal_type >= NAL_SLICE && i_nal_type <= NAL_SLICE_IDR )
    {
        slice_t slice;
        bool  b_new_picture;

        ParseSlice( p_dec, &b_new_picture, &slice, i_nal_ref_idc, i_nal_type, p_frag );

        /* */
        if( b_new_picture && p_sys->b_slice )
            p_pic = OutputPicture( p_dec );

        /* */
        p_sys->slice = slice;
      p_sys->b_slice = true;
    }
    else if( i_nal_type == NAL_SPS )
    {
        if( p_sys->b_slice )
            p_pic = OutputPicture( p_dec );
        p_sys->b_frame_sps = true;

        PutSPS( p_dec, p_frag );

        /* Do not append the SPS because we will insert it on keyframes */
        p_frag = NULL;
    }
    else if( i_nal_type == NAL_PPS )
    {
        if( p_sys->b_slice )
            p_pic = OutputPicture( p_dec );
        p_sys->b_frame_pps = true;

        PutPPS( p_dec, p_frag );

        /* Do not append the PPS because we will insert it on keyframes */
        p_frag = NULL;
    }
    else if( i_nal_type == NAL_AU_DELIMITER ||
             i_nal_type == NAL_SEI ||
             ( i_nal_type >= 13 && i_nal_type <= 18 ) )
    {
        if( p_sys->b_slice )
            p_pic = OutputPicture( p_dec );

        /* Parse SEI for CC support */
        if( i_nal_type == NAL_SEI )
        {
            ParseSei( p_dec, p_frag );
        }
        else if( i_nal_type == NAL_AU_DELIMITER )
        {
            if( p_sys->p_frame && (p_sys->p_frame->i_flags & BLOCK_FLAG_PRIVATE_AUD) )
            {
                block_Release( p_frag );
                p_frag = NULL;
            }
            else
            {
                p_frag->i_flags |= BLOCK_FLAG_PRIVATE_AUD;
            }
        }
    }

    /* Append the block */
    if( p_frag )
        block_ChainAppend( &p_sys->p_frame, p_frag );

    *pb_used_ts = false;
    if( p_sys->i_frame_dts <= VLC_TS_INVALID &&
        p_sys->i_frame_pts <= VLC_TS_INVALID )
    {
        p_sys->i_frame_dts = i_frag_dts;
        p_sys->i_frame_pts = i_frag_pts;
        *pb_used_ts = true;
    }
    return p_pic;
}
#endif

// init driver
static int init(sh_video_t *sh){
	int i;   
	int i_sps, i_pps;
	bool b_dummy;
#if H264
	decoder_sys_t *p_sys = malloc(sizeof(decoder_sys_t)); 
#endif
	uint8_t *extradata = (uint8_t *)(sh->bih + 1);
    	int extradata_size = sh->bih->biSize - sizeof(*sh->bih);
#if H264
	uint8_t *p =  &((uint8_t*)extradata)[4];   
	p_sys->i_avcC_length_size = 1 + ((*p++)&0x03);
	
	// SPS
	i_sps = (*p++)&0x1f;
	 for( i = 0; i < i_sps; i++ )
        {
            uint16_t i_length = GetWBE( p ); p += 2;
            if( i_length >
                (uint8_t*)extradata + extradata_size - p )
            {
                __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error in extradata SPS");
            }
            block_t *p_sps = CreateAnnexbNAL(  p, i_length );
            if( !p_sps )
                return 0;
            ParseNALBlock( &b_dummy, p_sps );
            p += i_length;
        }

	// PPS
	 i_pps = *p++;
        for( i = 0; i < i_pps; i++ )
        {
            uint16_t i_length = GetWBE( p ); p += 2;
            if( i_length >
                (uint8_t*)extradata + extradata_size - p )
            {
           	    __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","Error in extradata PPS");
            }
            block_t *p_pps = CreateAnnexbNAL( p, i_length );
            if( !p_pps )
                return 0;
            ParseNALBlock( &b_dummy, p_pps );
            p += i_length;
        }
         __android_log_print(ANDROID_LOG_ERROR,"MPlayer", "avcC length size=%d, sps=%d, pps=%d",p_sys->i_avcC_length_size, i_sps, i_pps );

#endif



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
	outBufferParseVideo[0]->nFilledLen = extradata_size+4;
outBufferParseVideo[0]->nFlags = OMX_BUFFERFLAG_CODECCONFIG;	
outBufferParseVideo[0]->nOffset = 0;
memset(outBufferParseVideo[0]->pBuffer,0x0,3);
        outBufferParseVideo[0]->pBuffer[3] = 0x01;
memcpy(outBufferParseVideo[0]->pBuffer+4, extradata, extradata_size);
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
// decode a frame
static mp_image_t* decode(sh_video_t *sh,void* data,int len,int flags){
mp_image_t *mpi;
i++;
 //       __android_log_print(ANDROID_LOG_ERROR  , "MPlayer","len=%d",len);

 if(!data || len <= 0 )
{ 
//mp_msg(MSGT_VO,MSGL_ERR,"No Len\n");
  return NULL;
}
	memset(outBufferParseVideo[0]->pBuffer,0x0,3);
	outBufferParseVideo[0]->pBuffer[3] = 0x01;
	memcpy(outBufferParseVideo[0]->pBuffer+4, data, len );
	outBufferParseVideo[0]->nFilledLen=len+4;
	outBufferParseVideo[0]->nTimeStamp=sh->timer*90000.0;
	 outBufferParseVideo[0]->nOffset = 0;
         outBufferParseVideo[0]->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;
	OMX_EmptyThisBuffer(omx_handle, outBufferParseVideo[0]);
	
/* 
mpi = mpcodecs_get_image(sh,MP_IMGTYPE_EXPORT,MP_IMGFLAG_ACCEPT_STRIDE,sh->disp_w,sh->disp_h);
   if(!mpi){
 //  mp_msg(MSGT_VO,MSGL_ERR,"No MPI\n");
	 return NULL;
   }	
*/
	return NULL;
}

