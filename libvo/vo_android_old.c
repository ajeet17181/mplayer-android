/*
 * based on video_out_null.c from mpeg2dec
 *
 * Copyright (C) Aaron Holtzman - June 2000
 *
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
#include <string.h>
#include <errno.h>
#include "config.h"
#include "mp_msg.h"
#include "help_mp.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "sub/sub.h"

static const vo_info_t info =
{
	"android video output",
	"android",
	"ajeet vijayvergiya <ajeet.vijay@gmail.com>",
	""
};

const LIBVO_EXTERN(android)
static unsigned char *ImageData=NULL;
static unsigned char *ImageData1=NULL;

static uint32_t image_width, image_height;





//static uint32_t
static int draw_slice(uint8_t *image[], int stride[], int w,int h,int x,int y)
//draw_slice(uint8_t *src[], uint32_t slice_num)
{
mp_msg(MSGT_VO,MSGL_ERR,"Draw slice\n");
	return 0;
}

static void draw_alpha(int x0, int y0, int w, int h, unsigned char *src,
                       unsigned char *srca, int stride)
{
vo_draw_alpha_rgb16(w,h,src,srca,stride,ImageData1+2*(y0*image_width+x0),2*image_width);
}

static void draw_osd(void)
{
if (ImageData1)
 vo_draw_text(image_width,image_height, draw_alpha);
}

static init=0;
static void
flip_page(void)
{
//mp_msg(MSGT_VO,MSGL_ERR,"flip\n");
if (init==1){
	main_init(ImageData);
	init=2;
}
else
   main_flip();
}

static int
draw_frame(uint8_t *src[])
{
if (init==0){
ImageData=(unsigned char *)src;
ImageData1=(unsigned char *)src[0];
init=1;
}
//	main_flip(src);
	return 0;
}

static int
query_format(uint32_t format)
{
  return  VFCAP_CSP_SUPPORTED;
}

static int
config(uint32_t width, uint32_t height, uint32_t d_width, uint32_t d_height, uint32_t flags, char *title, uint32_t format)
{
	image_width = width;
	image_height = height;
//	dst=malloc(owidth*oheight*2);
	return nativeConfig(width,height);
}

static void
uninit(void)
{
	init=0;
	nativeDeconfig();	
}


static void check_events(void)
{
}

static int preinit(const char *arg)
{
    if(arg)
    {
	mp_msg(MSGT_VO,MSGL_WARN, MSGTR_LIBVO_NULL_UnknownSubdevice,arg);
	return ENOSYS;
    }
    return 0;
}

static int control(uint32_t request, void *data, ...)
{
  switch (request) {
  case VOCTRL_QUERY_FORMAT:
    return query_format(*((uint32_t*)data));
  }
  return VO_NOTIMPL;
}
