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
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "mp_msg.h"
#include "vd_internal.h"
#include "libavutil/intreadwrite.h"
}
#include "pvavcdecoder.h"
static vd_info_t info =
{
	"H264 video decoder",
	"h264",
	"ajeet vijayvergiya",
	"ajeet.vijay@gmail.com",
	"H264 decoding"
};

#define MB_BASED_DEBLOCK

typedef enum {
  SPS,
  PPS,
  SLICE
} DEC_STATE;

 PVAVCDecoder *decoder;
  int parserInitialized = 0;
  int decoderInitialized = 0;

  uint8*        aInputBuf;

  DEC_STATE     state;
  AVCFrameIO outVid;



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
int status,i;
  state = SPS;
   uint8_t *extradata = (uint8_t *)(sh->bih + 1);
    int extradata_size = sh->bih->biSize - sizeof(*sh->bih);
	unsigned char *p = extradata;   
for (i=0;i<extradata_size;i++){
  mp_msg(MSGT_VO,MSGL_ERR,"[%x]",extradata[i]);
}
decoder = PVAVCDecoder::New();
        p += 8;

if (status=(decoder->DecodeSPS(p,22))==AVCDEC_SUCCESS){
   	mp_msg(MSGT_VO,MSGL_ERR,"SPS %x\n",*(p));
        state = PPS;
	p+=25;
      } else {
        mp_msg(MSGT_VO,MSGL_ERR,"No SPS %d,%x\n",status,AV_RB16(p) + 2);
        return 0;
      }
      if (status=(decoder->DecodePPS(p,4))==AVCDEC_SUCCESS){
        state = SLICE;
      } else {
   	mp_msg(MSGT_VO,MSGL_ERR,"No PPS %d,%x,%x,%x\n",status,*(p),*(p+1),p[0] & 0x1F);
        //return 0;
      }
	if(!mpcodecs_config_vo(sh,sh->disp_w,sh->disp_h,sh->format)) return 0;
  return (decoder!=NULL)?1:0;
}

// uninit driver
static void uninit(sh_video_t *sh){
}
static int inited=0;
// decode a frame
static mp_image_t* decode(sh_video_t *sh,void* data,int len,int flags){
mp_image_t *mpi;

int32 size=0;
size=len;
int32 status;
int           indexFrame;
int           releaseFrame;


 if(!data || len <= 0 ||  flags&2)
{ 
mp_msg(MSGT_VO,MSGL_ERR,"No Len\n");
  return NULL;
}

 mpi = mpcodecs_get_image(sh,MP_IMGTYPE_EXPORT,MP_IMGFLAG_ACCEPT_STRIDE,sh->disp_w,sh->disp_h);
   if(!mpi){
   mp_msg(MSGT_VO,MSGL_ERR,"No MPI\n");
	 return NULL;
   }	


      if ((status=decoder->DecodeAVCSlice((uint8*)data,&size))>AVCDEC_FAIL){
          decoder->GetDecOutput(&indexFrame,&releaseFrame,&outVid);

          if (releaseFrame == 1){
            decoder->AVC_FrameUnbind(indexFrame);
          }

	mpi->planes[0]= outVid.YCbCr[0];
	mpi->planes[1]= outVid.YCbCr[1];
   	mpi->planes[2]= outVid.YCbCr[2];
        mpi->stride[0] =  outVid.pitch;
        mpi->stride[1] =  mpi->stride[2] = outVid.pitch/2;
	return mpi;
    }
	else
	{
	mp_msg(MSGT_VO,MSGL_ERR,"Slice failed %d,%d\n",status,len);	
	}
}

