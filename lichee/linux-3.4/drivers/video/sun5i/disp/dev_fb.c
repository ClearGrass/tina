#include "drv_disp_i.h"
#include "dev_disp.h"
#include <linux/file.h>
#include <linux/sw_sync.h>

#define MY_BYTE_ALIGN(x) ( ( (x + (4*1024-1)) >> 12) << 12)             /* alloc based on 4K byte */
#define _ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

struct __fb_addr_para {
        int fb_paddr;
        int fb_size;
};
typedef struct
{
    int x;
    int y;
    int bit;
    void *buffer;
}sunxi_bmp_store_t;
typedef struct bmp_color_table_entry {
    __u8    blue;
    __u8    green;
    __u8    red;
    __u8    reserved;
} __attribute__ ((packed)) bmp_color_table_entry_t;

typedef struct bmp_header {
    /* Header */
    char signature[2];
    __u32   file_size;
    __u32   reserved;
    __u32   data_offset;
    /* InfoHeader */
    __u32   size;
    __u32   width;
    __u32   height;
    __u16   planes;
    __u16   bit_count;
    __u32   compression;
    __u32   image_size;
    __u32   x_pixels_per_m;
    __u32   y_pixels_per_m;
    __u32   colors_used;
    __u32   colors_important;
    /* ColorTable */

} __attribute__ ((packed)) bmp_header_t;

typedef struct bmp_image {
    bmp_header_t header;
    /* We use a zero sized array just as a placeholder for variable
       sized array */
    bmp_color_table_entry_t color_table[0];
} bmp_image_t;
extern fb_info_t g_fbi;
struct   mutex	gcommit_mutek;
static struct __fb_addr_para global_fb_addr;
struct sock *nl_sk = NULL;
#define MAX_MSGSIZE 256


#define FBHANDTOID(handle)  ((handle) - 100)
#define FBIDTOHAND(ID)  ((ID) + 100)


//              0:ARGB    1:BRGA    2:ABGR    3:RGBA
//seq           ARGB        BRGA       ARGB       BRGA
//br_swqp    0              0            1              1      
__s32 parser_disp_init_para(__disp_init_t * init_para)
{
    int  value;
    int  i;

    memset(init_para, 0, sizeof(__disp_init_t));
    
    if(OSAL_Script_FetchParser_Data("disp_init", "disp_init_enable", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.disp_init_enable fail\n");
        return -1;
    }
    init_para->b_init = value;

    if(OSAL_Script_FetchParser_Data("disp_init", "disp_mode", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.disp_mode fail\n");
        return -1;
    }
    init_para->disp_mode= value;

//screen0
    if(OSAL_Script_FetchParser_Data("disp_init", "screen0_output_type", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.screen0_output_type fail\n");
        return -1;
    }
    if(value == 0)
    {
        init_para->output_type[0] = DISP_OUTPUT_TYPE_NONE;
    }
    else if(value == 1)
    {
        init_para->output_type[0] = DISP_OUTPUT_TYPE_LCD;
    }
    else if(value == 2)
    {
        init_para->output_type[0] = DISP_OUTPUT_TYPE_TV;
    }
    else if(value == 3)
    {
        init_para->output_type[0] = DISP_OUTPUT_TYPE_HDMI;
    }
    else if(value == 4)
    {
        init_para->output_type[0] = DISP_OUTPUT_TYPE_VGA;
    }
    else
    {
        __wrn("invalid screen0_output_type %d\n", init_para->output_type[0]);
        return -1;
    }
    
    if(OSAL_Script_FetchParser_Data("disp_init", "screen0_output_mode", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.screen0_output_mode fail\n");
        return -1;
    }
    if(init_para->output_type[0] == DISP_OUTPUT_TYPE_TV || init_para->output_type[0] == DISP_OUTPUT_TYPE_HDMI)
    {
        init_para->tv_mode[0]= (__disp_tv_mode_t)value;
    }
    else if(init_para->output_type[0] == DISP_OUTPUT_TYPE_VGA)
    {
        init_para->vga_mode[0]= (__disp_vga_mode_t)value;
    }

//screen1
    if(OSAL_Script_FetchParser_Data("disp_init", "screen1_output_type", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.screen1_output_type fail\n");
        return -1;
    }
    if(value == 0)
    {
        init_para->output_type[1] = DISP_OUTPUT_TYPE_NONE;
    }
    else if(value == 1)
    {
        init_para->output_type[1] = DISP_OUTPUT_TYPE_LCD;
    }
    else if(value == 2)
    {
        init_para->output_type[1] = DISP_OUTPUT_TYPE_TV;
    }
    else if(value == 3)
    {
        init_para->output_type[1] = DISP_OUTPUT_TYPE_HDMI;
    }
    else if(value == 4)
    {
        init_para->output_type[1] = DISP_OUTPUT_TYPE_VGA;
    }
    else
    {
        __wrn("invalid screen1_output_type %d\n", init_para->output_type[1]);
        return -1;
    }
    
    if(OSAL_Script_FetchParser_Data("disp_init", "screen1_output_mode", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.screen1_output_mode fail\n");
        return -1;
    }
    if(init_para->output_type[1] == DISP_OUTPUT_TYPE_TV || init_para->output_type[1] == DISP_OUTPUT_TYPE_HDMI)
    {
        init_para->tv_mode[1]= (__disp_tv_mode_t)value;
    }
    else if(init_para->output_type[1] == DISP_OUTPUT_TYPE_VGA)
    {
        init_para->vga_mode[1]= (__disp_vga_mode_t)value;
    }

//fb0
    if(OSAL_Script_FetchParser_Data("disp_init", "fb0_framebuffer_num", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb0_framebuffer_num fail\n");
        return -1;
    }
    init_para->buffer_num[0]= value;

    if(OSAL_Script_FetchParser_Data("disp_init", "fb0_format", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb0_format fail\n");
        return -1;
    }
    init_para->format[0]= value;

    if(OSAL_Script_FetchParser_Data("disp_init", "fb0_pixel_sequence", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb0_pixel_sequence fail\n");
        return -1;
    }
    init_para->seq[0]= value;

    if(OSAL_Script_FetchParser_Data("disp_init", "fb0_scaler_mode_enable", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb0_scaler_mode_enable fail\n");
        return -1;
    }
    init_para->scaler_mode[0]= value;

//fb1
    if(OSAL_Script_FetchParser_Data("disp_init", "fb1_framebuffer_num", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb1_framebuffer_num fail\n");
        return -1;
    }
    init_para->buffer_num[1]= value;

    if(OSAL_Script_FetchParser_Data("disp_init", "fb1_format", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb1_format fail\n");
        return -1;
    }
    init_para->format[1]= value;

    if(OSAL_Script_FetchParser_Data("disp_init", "fb1_pixel_sequence", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb1_pixel_sequence fail\n");
        return -1;
    }
    init_para->seq[1]= value;

    if(OSAL_Script_FetchParser_Data("disp_init", "fb1_scaler_mode_enable", &value, 1) < 0)
    {
        __wrn("fetch script data disp_init.fb1_scaler_mode_enable fail\n");
        return -1;
    }
    init_para->scaler_mode[1]= value;


    __inf("====display init para begin====\n");
    __inf("b_init:%d\n", init_para->b_init);
    __inf("disp_mode:%d\n\n", init_para->disp_mode);
    for(i=0; i<2; i++)
    {
        __inf("output_type[%d]:%d\n", i, init_para->output_type[i]);
        __inf("tv_mode[%d]:%d\n", i, init_para->tv_mode[i]);
        __inf("vga_mode[%d]:%d\n\n", i, init_para->vga_mode[i]);
    }
    for(i=0; i<2; i++)
    {
        __inf("buffer_num[%d]:%d\n", i, init_para->buffer_num[i]);
        __inf("format[%d]:%d\n", i, init_para->format[i]);
        __inf("seq[%d]:%d\n", i, init_para->seq[i]);
        __inf("br_swap[%d]:%d\n", i, init_para->br_swap[i]);
        __inf("b_scaler_mode[%d]:%d\n", i, init_para->scaler_mode[i]);
    }
    __inf("====display init para end====\n");

    return 0;
}

void *Fb_map_kernel(unsigned long phys_addr, unsigned long size)
{
    int npages = PAGE_ALIGN(size) / PAGE_SIZE;
    struct page **pages = vmalloc(sizeof(struct page *) * npages);
    struct page **tmp = pages;
    struct page *cur_page = phys_to_page(phys_addr);
    pgprot_t pgprot;
    void *vaddr;
    int i;

    if(!pages)
        return 0;

    for(i = 0; i < npages; i++)
        *(tmp++) = cur_page++;

    pgprot = pgprot_noncached(PAGE_KERNEL);
    vaddr = vmap(pages, npages, VM_MAP, pgprot);

    vfree(pages);
    return vaddr;
}
static void Fb_unmap_kernel(void *vaddr)
{
    vunmap(vaddr);
}
__s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var)
{
    __u32 i=0, j=0;
    
    for(i = 0; i<height; i++)
    {
        for(j = 0; j<width/4; j++)
        {   
            __u32 offset = 0;

            if(var->bits_per_pixel == 32)
            {
                offset = width * i + j;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset));

                offset = width * i + j + width/4;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->green.length)-1)<<var->green.offset));

                offset = width * i + j + width/4*2;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->blue.length)-1)<<var->blue.offset));

                offset = width * i + j + width/4*3;
                sys_put_wvalue(base + offset*4, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset) | (((1<<var->green.length)-1)<<var->green.offset));
            }
            else if(var->bits_per_pixel == 16)
            {
                offset = width * i + j;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset));

                offset = width * i + j + width/4;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->green.length)-1)<<var->green.offset));

                offset = width * i + j + width/4*2;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->blue.length)-1)<<var->blue.offset));

                offset = width * i + j + width/4*3;
                sys_put_hvalue(base + offset*2, (((1<<var->transp.length)-1)<<var->transp.offset) | (((1<<var->red.length)-1)<<var->red.offset) | (((1<<var->green.length)-1)<<var->green.offset));
            }
        }
    }

    return 0;
}

__s32 fb_draw_gray_pictures(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var)
{
    __u32 time = 0;

    for(time = 0; time<18; time++)
    {
        __u32 i=0, j=0;
        
        for(i = 0; i<height; i++)
        {
            for(j = 0; j<width; j++)
            {   
                __u32 addr = base + (i*width+ j)*4;
                __u32 value = (0xff<<24) | ((time*15)<<16) | ((time*15)<<8) | (time*15);

                sys_put_wvalue(addr, value);
            }
        }
        OSAL_PRINTF("----%d\n", time*15);
        msleep(1000 * 5);
    }
    return 0;
}

static int __init Fb_map_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);
	struct page *page;
	
	page = alloc_pages(GFP_KERNEL,get_order(map_size));
	if(page != NULL)
	{
		info->screen_base = page_address(page);
		info->fix.smem_start = virt_to_phys(info->screen_base);
		memset(info->screen_base,0,info->fix.smem_len);
		__inf("Fb_map_video_memory, pa=0x%08lx size:0x%x\n",info->fix.smem_start, info->fix.smem_len);
		return 0;
	}
	else
	{
		__wrn("alloc_pages fail!\n");
		return -ENOMEM;
	}
#else        
	info->screen_base = (char __iomem *)disp_malloc(info->fix.smem_len, (__u32 *)(&info->fix.smem_start));
	if(info->screen_base)	{
		__inf("Fb_map_video_memory(reserve), pa=0x%x size:0x%x\n",(unsigned int)info->fix.smem_start, (unsigned int)info->fix.smem_len);
		memset(info->screen_base,0x0,info->fix.smem_len);

		global_fb_addr.fb_paddr = (unsigned int)info->fix.smem_start;
		global_fb_addr.fb_size=info->fix.smem_len;

		return 0;
	} else {
		__wrn("disp_malloc fail!\n");
		return -ENOMEM;
	}

	return 0;
#endif
}


static inline void Fb_unmap_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);
	
	free_pages((unsigned long)info->screen_base,get_order(map_size));
#else
	disp_free((void *)info->screen_base, (void*)info->fix.smem_start, info->fix.smem_len);
#endif
}

void sun5i_get_fb_addr_para(struct __fb_addr_para *fb_addr_para){

        if(fb_addr_para){
                fb_addr_para->fb_paddr = global_fb_addr.fb_paddr;
                fb_addr_para->fb_size  = global_fb_addr.fb_size;
        }
}

EXPORT_SYMBOL(sun5i_get_fb_addr_para);


__s32 disp_fb_to_var(__disp_pixel_fmt_t format, __disp_pixel_seq_t seq, __bool br_swap, struct fb_var_screeninfo *var)//todo
{
    if(format==DISP_FORMAT_ARGB8888)
    {
        var->bits_per_pixel = 32;
        var->transp.length = 8;
        var->red.length = 8;
        var->green.length = 8;
        var->blue.length = 8;
        if(seq == DISP_SEQ_ARGB && br_swap == 0)//argb
        {
            var->blue.offset = 0;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
            var->transp.offset = var->red.offset + var->red.length;
        }
        else if(seq == DISP_SEQ_BGRA && br_swap == 0)//bgra
        {           
            var->transp.offset = 0;
            var->red.offset = var->transp.offset + var->transp.length;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
        }
        else if(seq == DISP_SEQ_ARGB && br_swap == 1)//abgr
        {
            var->red.offset = 0;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
            var->transp.offset = var->blue.offset + var->blue.length;
        }
        else if(seq == DISP_SEQ_BGRA && br_swap == 1)//rgba
        {
            var->transp.offset = 0;
            var->blue.offset = var->transp.offset + var->transp.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
        }
    }
    else if(format==DISP_FORMAT_RGB888)
    {
        var->bits_per_pixel = 24;
        var->transp.length = 0;
        var->red.length = 8;
        var->green.length = 8;
        var->blue.length = 8;
        if(br_swap == 0)//rgb
        {
            var->blue.offset = 0;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
        }
        else//bgr
        {
            var->red.offset = 0;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
        }
    }
    else if(format==DISP_FORMAT_RGB655)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 0;
        var->red.length = 6;
        var->green.length = 5;
        var->blue.length = 5;
        if(br_swap == 0)//rgb
        {
            var->blue.offset = 0;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
        }
        else//bgr
        {
            var->red.offset = 0;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
        }
    }
    else if(format==DISP_FORMAT_RGB565)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 0;
        var->red.length = 5;
        var->green.length = 6;
        var->blue.length = 5;
        if(br_swap == 0)//rgb
        {
            var->blue.offset = 0;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
        }
        else//bgr
        {
            var->red.offset = 0;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
        }
    }
    else if(format==DISP_FORMAT_RGB556)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 0;
        var->red.length = 5;
        var->green.length = 5;
        var->blue.length = 6;
        if(br_swap == 0)//rgb
        {
            var->blue.offset = 0;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
        }
        else//bgr
        {
            var->red.offset = 0;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->blue.offset + var->blue.length;
        }
    }
    else if(format==DISP_FORMAT_ARGB1555)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 1;
        var->red.length = 5;
        var->green.length = 5;
        var->blue.length = 5;
        if(br_swap == 0)//rgb
        {
            var->blue.offset = 0;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
            var->transp.offset = var->red.offset + var->red.length;
        }
        else//bgr
        {
            var->red.offset = 0;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
            var->transp.offset = var->blue.offset + var->blue.length;
        }
    }
    else if(format==DISP_FORMAT_RGBA5551)
    {
        var->bits_per_pixel = 16;
        var->red.length = 5;
        var->green.length = 5;
        var->blue.length = 5;
        var->transp.length = 1;
        if(br_swap == 0)//rgba
        {
            var->transp.offset = 0;
            var->blue.offset = var->transp.offset + var->transp.length;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
        }
        else//bgra
        {
            var->transp.offset = 0;
            var->red.offset = var->transp.offset + var->transp.length;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
        }
    }
    else if(format==DISP_FORMAT_ARGB4444)
    {
        var->bits_per_pixel = 16;
        var->transp.length = 4;
        var->red.length = 4;
        var->green.length = 4;
        var->blue.length = 4;
        if(br_swap == 0)//argb
        {
            var->blue.offset = 0;
            var->green.offset = var->blue.offset + var->blue.length;
            var->red.offset = var->green.offset + var->green.length;
            var->transp.offset = var->red.offset + var->red.length;
        }
        else//abgr
        {
            var->red.offset = 0;
            var->green.offset = var->red.offset + var->red.length;
            var->blue.offset = var->green.offset + var->green.length;
            var->transp.offset = var->blue.offset + var->blue.length;
        }
    }

    return 0;
}

__s32 var_to_disp_fb(__disp_fb_t *fb, struct fb_var_screeninfo *var, struct fb_fix_screeninfo * fix)//todo
{    
    if(var->nonstd == 0)//argb
    {
		var->reserved[0] = DISP_MOD_INTERLEAVED;
		var->reserved[1] = DISP_FORMAT_ARGB8888;
		var->reserved[2] = DISP_SEQ_ARGB;
		var->reserved[3] = 0;

		switch (var->bits_per_pixel) 
		{
		case 1:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 1;
			var->reserved[1] = DISP_FORMAT_1BPP;
			break;

		case 2:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 2;
			var->reserved[1] = DISP_FORMAT_2BPP;
			break;

		case 4:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 4;
			var->reserved[1] = DISP_FORMAT_4BPP;
			break;
			
		case 8:
		    var->red.offset = var->green.offset = var->blue.offset	= 0;
			var->red.length	= var->green.length = var->blue.length	= 8;
			var->reserved[1] = DISP_FORMAT_8BPP;
			break;
						
		case 16:
			if(var->red.length==6 && var->green.length==5 && var->blue.length==5)
			{
			    var->reserved[1] = DISP_FORMAT_RGB655;
			    if(var->red.offset == 10 && var->green.offset == 5 && var->blue.offset == 0)//rgb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->blue.offset == 11 && var->green.offset == 6 && var->red.offset == 0)//bgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid RGB655 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
				
			}
			else if(var->red.length==5 && var->green.length==6 && var->blue.length==5)
			{
				var->reserved[1] = DISP_FORMAT_RGB565;
				if(var->red.offset == 11 && var->green.offset == 5 && var->blue.offset == 0)//rgb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->blue.offset == 11 && var->green.offset == 5 && var->red.offset == 0)//bgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid RGB565 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else if(var->red.length==5 && var->green.length==5 && var->blue.length==6)
			{
				var->reserved[1] = DISP_FORMAT_RGB556;
				if(var->red.offset == 11 && var->green.offset == 6 && var->blue.offset == 0)//rgb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->blue.offset == 10 && var->green.offset == 5 && var->red.offset == 0)//bgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid RGB556 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else if(var->transp.length==1 && var->red.length==5 && var->green.length==5 && var->blue.length==5)
			{
				var->reserved[1] = DISP_FORMAT_ARGB1555;
				if(var->transp.offset == 15 && var->red.offset == 10 && var->green.offset == 5 && var->blue.offset == 0)//argb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->transp.offset == 15 && var->blue.offset == 10 && var->green.offset == 5 && var->red.offset == 0)//abgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid ARGB1555 format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else if(var->transp.length==4 && var->red.length==4 && var->green.length==4 && var->blue.length==4)
			{
				var->reserved[1] = DISP_FORMAT_ARGB4444;
				if(var->transp.offset == 12 && var->red.offset == 8 && var->green.offset == 4 && var->blue.offset == 0)//argb
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			    else if(var->transp.offset == 12 && var->blue.offset == 8 && var->green.offset == 4 && var->red.offset == 0)//abgr
			    {
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 1;
			    }
			    else
			    {
			        __wrn("invalid ARGB4444 format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
			        var->reserved[2] = DISP_SEQ_ARGB;
			        var->reserved[3] = 0;
			    }
			}
			else
			{
			    __wrn("invalid bits_per_pixel :%d\n", var->bits_per_pixel);
				return -EINVAL;
			}
			break;
			
		case 24:
			var->red.length		= 8;
			var->green.length	= 8;
			var->blue.length	= 8;
			var->reserved[1] = DISP_FORMAT_RGB888;
			if(var->red.offset == 16 && var->green.offset == 8 && var->blue.offset == 0)//rgb
		    {
		        var->reserved[2] = DISP_SEQ_ARGB;
		        var->reserved[3] = 0;
		    }
		    else if(var->blue.offset == 16 && var->green.offset == 8&& var->red.offset == 0)//bgr
		    {
		        var->reserved[2] = DISP_SEQ_ARGB;
		        var->reserved[3] = 1;
		    }
		    else
		    {
		        __wrn("invalid RGB888 format<red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->red.offset,var->green.offset,var->blue.offset);
		        var->reserved[2] = DISP_SEQ_ARGB;
		        var->reserved[3] = 0;
		    }
			break;
			
		case 32:
			var->transp.length  = 8;
			var->red.length		= 8;
			var->green.length	= 8;
			var->blue.length	= 8;
			var->reserved[1] = DISP_FORMAT_ARGB8888;

			if(var->red.offset == 16 && var->green.offset == 8 && var->blue.offset == 0)//argb
			{
			    var->reserved[2] = DISP_SEQ_ARGB;
			    var->reserved[3] = 0;
 			}
			else if(var->blue.offset == 24 && var->green.offset == 16 && var->red.offset == 8)//bgra
			{
			    var->reserved[2] = DISP_SEQ_BGRA;
			    var->reserved[3] = 0;
			}
			else if(var->blue.offset == 16 && var->green.offset == 8 && var->red.offset == 0)//abgr
			{
			    var->reserved[2] = DISP_SEQ_ARGB;
			    var->reserved[3] = 1;
			}
			else if(var->red.offset == 24 && var->green.offset == 16 && var->blue.offset == 8)//rgba
			{
			    var->reserved[2] = DISP_SEQ_BGRA;
			    var->reserved[3] = 1;
			}
			else
			{
			    __wrn("invalid argb format<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",var->transp.offset,var->red.offset,var->green.offset,var->blue.offset);
			    var->reserved[2] = DISP_SEQ_ARGB;
			    var->reserved[3] = 0;
			}
			break;
			
		default:
		    __wrn("invalid bits_per_pixel :%d\n", var->bits_per_pixel);
			return -EINVAL;
		}
	}

    fb->mode = var->reserved[0];
    fb->format = var->reserved[1];
    fb->seq = var->reserved[2];
    fb->br_swap = var->reserved[3];
    fb->size.width = var->xres_virtual;
    
    fix->line_length = (var->xres_virtual * var->bits_per_pixel) / 8;
	
	return 0;
}


static int Fb_open(struct fb_info *info, int user)
{
	return 0;
}
static int Fb_release(struct fb_info *info, int user)
{
	return 0;
}
static int Fb_wait_for_vsync(struct fb_info *);
static int Fb_pan_display(struct fb_var_screeninfo *var,struct fb_info *info)
{
	__u32 sel = 0;

	//__inf("Fb_pan_display\n");

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
            || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)))
        {
            __s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
            __disp_layer_info_t layer_para;
            __u32 buffer_num = 1;
            __u32 y_offset = 0;

            if(g_fbi.fb_mode[info->node] == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS)
            {
                if(sel != var->reserved[0])
                {
                    return -1;
                }
            }

            if(g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB)
            {   
                buffer_num = 2;
            }
            if((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB))
            {
                y_offset = var->yres / 2;
            }
            
        	BSP_disp_layer_get_para(sel, layer_hdl, &layer_para);

        	if(layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)
        	{
            	layer_para.src_win.x = var->xoffset;
            	layer_para.src_win.y = var->yoffset + y_offset;
            	layer_para.src_win.width = var->xres;
            	layer_para.src_win.height = var->yres / buffer_num;

            	BSP_disp_layer_set_src_window(sel, layer_hdl, &(layer_para.src_win));
            }
            else
            {
            	layer_para.src_win.x = var->xoffset;
            	layer_para.src_win.y = var->yoffset + y_offset;
            	layer_para.src_win.width = var->xres;
            	layer_para.src_win.height = var->yres / buffer_num;

            	layer_para.scn_win.width = var->xres;
            	layer_para.scn_win.height = var->yres / buffer_num;

                BSP_disp_layer_set_src_window(sel, layer_hdl, &(layer_para.src_win));
            	BSP_disp_layer_set_screen_window(sel, layer_hdl, &(layer_para.scn_win));
            }
        }
    }
    	Fb_wait_for_vsync(info);
	return 0;
}

static int Fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)//todo
{
	return 0;
}

static int Fb_set_par(struct fb_info *info)//todo
{
	__u32 sel = 0;
    
	__inf("Fb_set_par\n"); 

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
            || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)))
        {
            struct fb_var_screeninfo *var = &info->var;
            struct fb_fix_screeninfo * fix = &info->fix;
            __s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
            __disp_layer_info_t layer_para;
            __u32 buffer_num = 1;
            __u32 y_offset = 0;

            if(g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB)
            {   
                buffer_num = 2;
            }
            if((sel==0) && (g_fbi.fb_mode[info->node] == FB_MODE_DUAL_SAME_SCREEN_TB))
            {
                y_offset = var->yres / 2;
            }
            BSP_disp_layer_get_para(sel, layer_hdl, &layer_para);

            var_to_disp_fb(&(layer_para.fb), var, fix);
        	layer_para.src_win.x = var->xoffset;
        	layer_para.src_win.y = var->yoffset + y_offset;
        	layer_para.src_win.width = var->xres;
        	layer_para.src_win.height = var->yres / buffer_num;
        	if(layer_para.mode != DISP_LAYER_WORK_MODE_SCALER)
        	{
            	layer_para.scn_win.width = layer_para.src_win.width;
            	layer_para.scn_win.height = layer_para.src_win.height;
        	}
            BSP_disp_layer_set_para(sel, layer_hdl, &layer_para);
        }
    }
	return 0;
}
 

static int Fb_setcolreg(unsigned regno,unsigned red, unsigned green, unsigned blue,unsigned transp, struct fb_info *info)
{
    __u32 sel = 0;
    
	 __inf("Fb_setcolreg,regno=%d,a=%d,r=%d,g=%d,b=%d\n",regno, transp,red, green, blue); 

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
            || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)))
        {
            unsigned int val;

        	switch (info->fix.visual) 
        	{
        	case FB_VISUAL_PSEUDOCOLOR:
        		if (regno < 256) 
        		{
        			val = (transp<<24) | (red<<16) | (green<<8) | blue;
        			BSP_disp_set_palette_table(sel, &val, regno*4, 4);
        		}
        		break;

        	default:
        		break;
        	}
    	}
	}

	return 0;
}

static int Fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
    __u32 sel = 0;
    
	__inf("Fb_setcmap\n"); 
	
    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
            || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)))
        {
            unsigned int j = 0, val = 0;
            unsigned char hred, hgreen, hblue, htransp = 0xff;
            unsigned short *red, *green, *blue, *transp;

            red = cmap->red;
            green = cmap->green;
            blue = cmap->blue;
            transp = cmap->transp;
            
        	for (j = 0; j < cmap->len; j++) 
        	{
        		hred = (*red++)&0xff;
        		hgreen = (*green++)&0xff;
        		hblue = (*blue++)&0xff;
        		if (transp)
        		{
        			htransp = (*transp++)&0xff;
        		}
        		else
        		{
        		    htransp = 0xff;
        		}

        		val = (htransp<<24) | (hred<<16) | (hgreen<<8) |hblue;
        		BSP_disp_set_palette_table(sel, &val, (cmap->start + j) * 4, 4);
        	}
    	}
	}
	return 0;
}

int Fb_blank(int blank_mode, struct fb_info *info)
{    
    __u32 sel = 0;
    
	__inf("Fb_blank,mode:%d\n",blank_mode); 

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
            || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)))
        {
            __s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];

        	if (blank_mode == FB_BLANK_POWERDOWN) 
        	{
        		BSP_disp_layer_close(sel, layer_hdl);
        	} 
        	else 
        	{
        		BSP_disp_layer_open(sel, layer_hdl);
        	}
            //DRV_disp_wait_cmd_finish(sel);
        }
    }
	return 0;
}

static int Fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    __inf("Fb_cursor\n"); 

    return 0;
}

static int Fb_wait_for_vsync(struct fb_info *info)
{
	unsigned long count;
	__u32 sel = 0;
	int ret;

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
            || ((sel==1) && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)))
        {
            if(BSP_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_NONE)
            {
                return 0;
            }
            
        	count = g_fbi.wait_count[sel];
        	ret = wait_event_interruptible_timeout(g_fbi.wait[sel], count != g_fbi.wait_count[sel], msecs_to_jiffies(50));
        	if (ret == 0)
        	{
        	    __inf("timeout\n");
        		return -ETIMEDOUT;
        	}
        }
    }
    
	return 0;
}

__s32 DRV_disp_int_process(__u32 sel)
{
    g_fbi.wait_count[sel]++;
    wake_up_interruptible(&g_fbi.wait[sel]);

    return 0;
}

__s32 DRV_disp_vsync_event(__u32 sel)
{    	
    g_fbi.vsync_timestamp[sel] = ktime_get();
    schedule_work(&g_fbi.vsync_work[sel]);
    return 0;
}

static void send_vsync_work_0(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "VSYNC0=%llu",ktime_to_ns(g_fbi.vsync_timestamp[0]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&g_fbi.dev->kobj, KOBJ_CHANGE, envp);
}

static void send_vsync_work_1(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "VSYNC1=%llu",ktime_to_ns(g_fbi.vsync_timestamp[1]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&g_fbi.dev->kobj, KOBJ_CHANGE, envp);
}

#define _ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

static int dispc_update_regs(setup_dispc_data_t *psDispcData)
{
    __disp_layer_info_t layer_info;
    int i,disp,hdl;
    int start_idx, layer_num = 0;
    int err = 0;

    for(i=0; i<psDispcData->post2_layers; i++)
    {
		
	        if(psDispcData->acquireFence[i]) 
	        {
	            err = sync_fence_wait(psDispcData->acquireFence[i] ,1000);
	           
		    	sync_fence_put(psDispcData->acquireFence[i]);
			
		    	if (err < 0)
	            {
	                __wrn("synce_fence_wait() timeout, layer:%d, fd:%d\n", i, psDispcData->acquireFenceFd[i]);					
	                sw_sync_timeline_inc(g_fbi.timeline, 1);
					return -1;
	            }
	            
		    	            
		}
    }

	if(g_fbi.b_no_output == 1){
		sw_sync_timeline_inc(g_fbi.timeline, 1);
		return 0;
	}

	 BSP_disp_cmd_cache(0);
	//BSP_disp_cmd_cache(1);

    for(disp = 0; disp < 1; disp++)
    {
        if(!psDispcData->show_black[disp])
        {
            if(disp == 0)
            {
                start_idx = 0;
                layer_num = psDispcData->primary_display_layer_num;
            }
            else
            {
                start_idx = psDispcData->primary_display_layer_num;
                layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
            }

            for(i=0; i<4; i++)
            {
                hdl = 100 + i;
				memset( &layer_info,0,sizeof(__disp_layer_info_t));
                BSP_disp_layer_get_para(disp, hdl, &layer_info);
                if(layer_info.mode == DISP_LAYER_WORK_MODE_SCALER)
                {
                    if((i >= layer_num) || (psDispcData->layer_info[start_idx + i].mode == DISP_LAYER_WORK_MODE_NORMAL))
                    {
                        BSP_disp_layer_release(disp, hdl);
                        BSP_disp_layer_request(disp, DISP_LAYER_WORK_MODE_NORMAL);
                    }
                }
				
            }
        }
    }
    for(disp = 0; disp < 1; disp++)
    {
	if(!psDispcData->show_black[disp])
	{
	    int haveFbTarget = 0;
	    
	    if(disp == 0)
	    {
		start_idx = 0;
		layer_num = psDispcData->primary_display_layer_num;
	    }
	    else
	    {
		start_idx = psDispcData->primary_display_layer_num;
		layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
	    }
	    
	    for(i=0; i<4; i++)
	    {
			hdl = 100 + i;
			
			if(i < layer_num )
			{
			    memcpy(&layer_info, &psDispcData->layer_info[start_idx + i], sizeof(__disp_layer_info_t));

			    if(layer_info.fb.mode == DISP_MOD_NON_MB_PLANAR )
			    {
					if(layer_info.fb.format == DISP_FORMAT_YUV420)
					{
					   	//bsp_disp_set_farmeflag(0,layer_info.sourceFlag);

					    layer_info.fb.addr[2] = layer_info.fb.addr[0] + layer_info.fb.size.width * layer_info.fb.size.height;
					    layer_info.fb.addr[1] = layer_info.fb.addr[2] + (_ALIGN(layer_info.fb.size.width/2,16) * layer_info.fb.size.height)/2;
					}
			    }
				if(layer_info.fb.mode == DISP_MOD_NON_MB_UV_COMBINED)
			    {
					if(layer_info.fb.format == DISP_FORMAT_YUV420)
					{
					   	//bsp_disp_set_farmeflag(0,layer_info.sourceFlag);
					    layer_info.fb.addr[1] = layer_info.fb.addr[0] + layer_info.fb.size.width * layer_info.fb.size.height;
					}
			    }
			    BSP_disp_layer_set_para(disp, hdl, &layer_info);		   
			    BSP_disp_layer_set_top(disp, hdl);
			    BSP_disp_layer_open(disp, hdl);
			    //printk(KERN_WARNING "##update layer:%d addr:%x\n", psDispcData->primary_display_layer_num, layer_info.fb.addr[0]);

			    if(i==1 && (psDispcData->layer_info[start_idx + 1].prio < psDispcData->layer_info[start_idx].prio))
			    {
					haveFbTarget = 1;
			    }
			}
			else
			{
			    BSP_disp_layer_close(disp, hdl);
			}

			if(haveFbTarget)
			{
			    BSP_disp_layer_set_top(disp, 100);
			}
	    }
	}
	else
	{

	    for(i=0; i<4; i++)
	    {
			hdl = 100 + i;

			BSP_disp_layer_close(disp, hdl);
	    }
	}

    }
	BSP_disp_cmd_submit(0);
//	BSP_disp_cmd_submit(1);
	//spin_unlock(&(g_fbi.update_reg_lock));

    Fb_wait_for_vsync(g_fbi.fbinfo[0]);
   sw_sync_timeline_inc(g_fbi.timeline, 1);
    return 0;
}

static void hwc_commit_work(struct work_struct *work)
{
    dispc_data_list_t *data, *next;
    struct list_head saved_list;


    mutex_lock(&(gcommit_mutek));
	
    mutex_lock(&(g_fbi.update_regs_list_lock));
    list_replace_init(&g_fbi.update_regs_list, &saved_list);
   
    mutex_unlock(&(g_fbi.update_regs_list_lock));


    list_for_each_entry_safe(data, next, &saved_list, list) 
    {
         list_del(&data->list);
         dispc_update_regs(&data->hwc_data);
         kfree(data);

    }
	mutex_unlock(&(gcommit_mutek));
}

int hwc_commit(int sel, setup_dispc_data_t *disp_data)
{
	dispc_data_list_t *disp_data_list;
	struct sync_fence *fence;
	struct sync_pt *pt;
	int fd = -1;
	int i = 0;

	for(i=0; i<disp_data->post2_layers; i++)
	{

		if(disp_data->acquireFenceFd[i] >= 0)
		{
			disp_data->acquireFence[i] = sync_fence_fdget(disp_data->acquireFenceFd[i]);
			if(!disp_data->acquireFence[i]) 
			{
				printk("sync_fence_fdget()fail, layer:%d fd:%d\n", i, disp_data->acquireFenceFd[i]);
				return -1;
			}
		}
	}

	
	fd = get_unused_fd();
	if (fd < 0)
	{
	    return -1;
	}
	g_fbi.timeline_max++;
	pt = sw_sync_pt_create(g_fbi.timeline, g_fbi.timeline_max);
	fence = sync_fence_create("display", pt);
	sync_fence_install(fence, fd);

	disp_data_list = kzalloc(sizeof(dispc_data_list_t), GFP_KERNEL);
	memcpy(&disp_data_list->hwc_data, disp_data, sizeof(setup_dispc_data_t));
	mutex_lock(&(g_fbi.update_regs_list_lock));
	list_add_tail(&disp_data_list->list, &g_fbi.update_regs_list);
	mutex_unlock(&(g_fbi.update_regs_list_lock));
	schedule_work(&g_fbi.commit_work);	
	//hwc_commit_work(0);
	
	return fd;
    
}
static int Fb_ioctl(struct fb_info *info, unsigned int cmd,unsigned long arg)
{
	long ret = 0;
	unsigned long layer_hdl = 0;

	switch (cmd) 
	{
    case FBIOGET_LAYER_HDL_0:
        if(g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1)
        {
            layer_hdl = g_fbi.layer_hdl[info->node][0];
            ret = copy_to_user((void __user *)arg, &layer_hdl, sizeof(unsigned long));
        }
        else
        {
            ret = -1;
        }
        break; 

    case FBIOGET_LAYER_HDL_1:
        if(g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)
        {
            layer_hdl = g_fbi.layer_hdl[info->node][1];
            ret = copy_to_user((void __user *)arg, &layer_hdl, sizeof(unsigned long));
        }
        else
        {
            ret = -1;
        }
        break; 

#if 0
    case FBIOGET_VBLANK:
    {
        struct fb_vblank vblank;
        __disp_tcon_timing_t tt;
        __u32 line = 0;
        __u32 sel;

        sel = (g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)?1:0;
        line = BSP_disp_get_cur_line(sel);
        BSP_disp_get_timming(sel, &tt);
        
        memset(&vblank, 0, sizeof(struct fb_vblank));
        vblank.flags |= FB_VBLANK_HAVE_VBLANK;
        vblank.flags |= FB_VBLANK_HAVE_VSYNC;
        if(line <= (tt.ver_total_time-tt.ver_pixels))
        {
            vblank.flags |= FB_VBLANK_VBLANKING;
        }
        if((line > tt.ver_front_porch) && (line < (tt.ver_front_porch+tt.ver_sync_time)))
        {
            vblank.flags |= FB_VBLANK_VSYNCING;
        }
        
        if (copy_to_user((void __user *)arg, &vblank, sizeof(struct fb_vblank)))
            ret = -EFAULT;

        break;
    }
#endif

    case FBIO_WAITFORVSYNC:
    {
                        //ret = Fb_wait_for_vsync(info);
        break;
    }

   	default:
   	    //__inf("not supported fb io cmd:%x\n", cmd);
		break;
	}
	return ret;
}

static struct fb_ops dispfb_ops = 
{
	.owner		    = THIS_MODULE,
	.fb_open        = Fb_open,
	.fb_release     = Fb_release,
	.fb_pan_display	= Fb_pan_display,
	.fb_ioctl       = Fb_ioctl,
	.fb_check_var   = Fb_check_var,
	.fb_set_par     = Fb_set_par,
	.fb_setcolreg   = Fb_setcolreg,
	.fb_setcmap     = Fb_setcmap,
	.fb_blank       = Fb_blank,
	.fb_cursor      = Fb_cursor,
};

/* Greatest common divisor of x and y */
static unsigned long GCD(unsigned long x, unsigned long y)
{
    while (y != 0)
    {
        unsigned long r = x % y;
        x = y;
        y = r;
    }
    return x;
}
 
/* Least common multiple of x and y */
static unsigned long LCM(unsigned long x, unsigned long y)
{
    unsigned long gcd = GCD(x, y);
    return (gcd == 0) ? 0 : ((x / gcd) * y);
}
 
/* Round x up to a multiple of y */
static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
    unsigned long div = x / y;
    unsigned long rem = x % y;
    return (div + ((rem == 0) ? 0 : 1)) * y;
}
static phys_addr_t bootlogo_addr = 0;
static int Fb_map_kernel_logo(__u32 sel, struct fb_info *info)
{
    void *vaddr = NULL;
    unsigned int paddr =  0;
    void *screen_offset = NULL, *image_offset = NULL;
    char *tmp_buffer = NULL;
    char *bmp_data =NULL;
    sunxi_bmp_store_t s_bmp_info;
    sunxi_bmp_store_t *bmp_info = &s_bmp_info;
    bmp_image_t *bmp = NULL;
    int zero_num = 0;
    unsigned long x, y, bmp_bpix, fb_width, fb_height;
    unsigned int effective_width, effective_height;
    unsigned int offset;
    int i = 0;

    paddr = bootlogo_addr;
    if(0 == paddr) {
        __wrn("Fb_map_kernel_logo failed!");
        return -1;
    }

    /* parser bmp header */
    offset = paddr & ~PAGE_MASK;
    vaddr = (void *)Fb_map_kernel(paddr, sizeof(bmp_header_t));
    if(0 == vaddr) {
        __wrn("fb_map_kernel failed, paddr=0x%x,size=0x%x\n", paddr, sizeof(bmp_header_t));
        return -1;
    }
    bmp = (bmp_image_t *)((unsigned int)vaddr + offset);
    if((bmp->header.signature[0]!='B') || (bmp->header.signature[1] !='M')) {
        __wrn("this is not a bmp picture\n");
        return -1;
    }

    bmp_bpix = bmp->header.bit_count/8;

    if((bmp_bpix != 3) && (bmp_bpix != 4)) {
        return -1;
    }

    if(bmp_bpix ==3) {
        zero_num = (4 - ((3*bmp->header.width) % 4))&3;
    }

    x = bmp->header.width;
    y = (bmp->header.height & 0x80000000) ? (-bmp->header.height):(bmp->header.height);
    fb_width = info->var.xres;
    fb_height = info->var.yres;
    if((paddr <= 0) || x <= 1 || y <= 1) {
        __wrn("kernel logo para error!\n");
        return -EINVAL;
    }

    bmp_info->x = x;
    bmp_info->y = y;
    bmp_info->bit = bmp->header.bit_count;
    bmp_info->buffer = (void *)(info->screen_base);

    if(bmp_bpix == 3)
        info->var.bits_per_pixel = 24;
    else if(bmp_bpix == 4)
        info->var.bits_per_pixel = 32;
    else
        info->var.bits_per_pixel = 32;

    Fb_unmap_kernel(vaddr);

    /* map the total bmp buffer */
    vaddr = (void *)Fb_map_kernel(paddr, x * y * bmp_bpix + sizeof(bmp_header_t));
    if(0 == vaddr) {
        __wrn("fb_map_kernel failed, paddr=0x%x,size=0x%x\n", paddr, (unsigned int)(x * y * bmp_bpix + sizeof(bmp_header_t)));
        return -1;
    }

    bmp = (bmp_image_t *)((unsigned int)vaddr + offset);

    tmp_buffer = (char *)bmp_info->buffer;
    screen_offset = (void *)bmp_info->buffer;
    bmp_data = (char *)(vaddr + offset +  bmp->header.data_offset);
    image_offset = (void *)bmp_data;
    effective_width = (fb_width<x)?fb_width:x;
    effective_height = (fb_height<y)?fb_height:y;

    if(bmp->header.height & 0x80000000) {
        if(fb_width > x) {
        screen_offset = (void *)((u32)info->screen_base + (fb_width * (abs(fb_height - y) / 2)
                + abs(fb_width - x) / 2) * (info->var.bits_per_pixel >> 3));
        } else if(fb_width < x) {
                image_offset = (void *)((u32)bmp_data + (x * ((y - fb_height) / 2)
                        + (x - fb_width) / 2) * (info->var.bits_per_pixel >> 3));
        }

        for(i=0; i<effective_height; i++) {
            memcpy((void*)screen_offset, image_offset, effective_width*(info->var.bits_per_pixel >> 3));
            screen_offset = (void*)((u32)screen_offset + fb_width*(info->var.bits_per_pixel >> 3));
            image_offset = (void *)image_offset + x * (info->var.bits_per_pixel >> 3);
        }
    }
    else {
        screen_offset = (void *)((u32)info->screen_base + (fb_width * (abs(fb_height - y) / 2)
                + abs(fb_width - x) / 2) * (info->var.bits_per_pixel >> 3));
        image_offset = (void *)((u32)image_offset + (x * (abs(y - fb_height) / 2)
                + abs(x - fb_width) / 2) * (info->var.bits_per_pixel >> 3));
#if 0
        if(3 == bmp_bpix) {
            unsigned char* ptemp = NULL;
            unsigned char temp = 0;
            int h=0;

            ptemp = (char *)image_offset;

            for(h=0; h<effective_height; h++) {
                for(i=0; i<=effective_width * (info->var.bits_per_pixel >> 3) - 3; i+=3) {
                    temp = ptemp[i];
                    ptemp[i] = ptemp[i+1];
                    ptemp[i+1] = temp;
                }
                ptemp = (char *)((void *)bmp_data + h * x *  (info->var.bits_per_pixel >> 3));
            }
        }
#endif
        image_offset = (void *)bmp_data + (effective_height-1) * x *  (info->var.bits_per_pixel >> 3);
        for(i=effective_height-1; i>=0; i--) {
            memcpy((void*)screen_offset, image_offset, effective_width*(info->var.bits_per_pixel >> 3));
            screen_offset = (void*)((u32)screen_offset + fb_width*(info->var.bits_per_pixel >> 3));
            image_offset = (void *)bmp_data + i * x *  (info->var.bits_per_pixel >> 3);
        }
    }

    Fb_unmap_kernel(vaddr);
    return 0;
}
__s32 Display_Fb_Request(__u32 fb_id, __disp_fb_create_para_t *fb_para)
{
	struct fb_info *info = NULL;
	__s32 hdl = 0;
	__disp_layer_info_t layer_para;
	__u32 sel;
	__u32 xres, yres;
	unsigned long ulLCM;
    
	__inf("Display_Fb_Request,fb_id:%d\n", fb_id);

    info = g_fbi.fbinfo[fb_id];

    xres = fb_para->width;
    yres = fb_para->height;
	
	info->var.xoffset       = 0;
	info->var.yoffset       = 0;
	info->var.xres          = xres;
	info->var.yres          = yres;
	info->var.xres_virtual  = xres;
#if 1   /*mali 16 Bytes aligned */
	info->fix.line_length   = (RoundUpToMultiple(fb_para->width,16) * info->var.bits_per_pixel) >> 3;
	ulLCM = LCM(info->fix.line_length, PAGE_SIZE);
	info->fix.smem_len      = RoundUpToMultiple(info->fix.line_length * RoundUpToMultiple(fb_para->height,16), ulLCM) * fb_para->buffer_num;
	info->var.yres_virtual  = info->fix.smem_len / info->fix.line_length;
#else
	info->var.yres_virtual  = yres * fb_para->buffer_num;
    info->fix.line_length   = (fb_para->width * info->var.bits_per_pixel) >> 3;
    info->fix.smem_len      = info->fix.line_length * fb_para->height * fb_para->buffer_num;
#endif
    Fb_map_video_memory(info);

    for(sel = 0; sel < 2; sel++)
    {
        if(((sel==0) && (fb_para->fb_mode != FB_MODE_SCREEN1))
        || ((sel==1) && (fb_para->fb_mode != FB_MODE_SCREEN0)))
        {
    	    __u32 y_offset = 0, src_width = xres, src_height = yres;

	        if(((sel==0) && (fb_para->fb_mode == FB_MODE_SCREEN0 || fb_para->fb_mode == FB_MODE_DUAL_SAME_SCREEN_TB))
                || ((sel==1) && (fb_para->fb_mode == FB_MODE_SCREEN1))
                || ((sel == fb_para->primary_screen_id) && (fb_para->fb_mode == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS)))
            {
                __disp_tcon_timing_t tt;

                if(BSP_disp_get_timming(sel, &tt) >= 0)
                {
                    info->var.pixclock = 1000000000 / tt.pixel_clk;
                    info->var.left_margin = tt.hor_back_porch;
                    info->var.right_margin = tt.hor_front_porch;
                    info->var.upper_margin = tt.ver_back_porch;
                    info->var.lower_margin = tt.ver_front_porch;
                    info->var.hsync_len = tt.hor_sync_time;
                    info->var.vsync_len = tt.ver_sync_time;
                }
            }
            Fb_map_kernel_logo(sel, info);
            if(fb_para->fb_mode == FB_MODE_DUAL_SAME_SCREEN_TB)
            {
                src_height = yres/ 2;
                if(sel == 0)
                {
                    y_offset = yres / 2;
                }
            }
            
            memset(&layer_para, 0, sizeof(__disp_layer_info_t));
            layer_para.mode = fb_para->mode;
            layer_para.scn_win.width = src_width;
            layer_para.scn_win.height = src_height;
            if(fb_para->fb_mode == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS)
            {
                if(sel != fb_para->primary_screen_id)
                {
                    layer_para.mode = DISP_LAYER_WORK_MODE_SCALER;
                    layer_para.scn_win.width = fb_para->aux_output_width;
                    layer_para.scn_win.height = fb_para->aux_output_height;
                }
                else if(fb_para->mode == DISP_LAYER_WORK_MODE_SCALER)
                {
                    layer_para.scn_win.width = fb_para->output_width;
                    layer_para.scn_win.height = fb_para->output_height;
                }
            }
            else if(fb_para->mode == DISP_LAYER_WORK_MODE_SCALER)
            {
                layer_para.scn_win.width = fb_para->output_width;
                layer_para.scn_win.height = fb_para->output_height;
            }
                            
            hdl = BSP_disp_layer_request(sel, layer_para.mode);
            
            layer_para.pipe = 0;
            layer_para.alpha_en = 1;
            layer_para.alpha_val = 0xff;
            layer_para.ck_enable = 0;
            layer_para.src_win.x = 0;
            layer_para.src_win.y = y_offset;
            layer_para.src_win.width = src_width;
            layer_para.src_win.height = src_height;
            layer_para.scn_win.x = 0;
            layer_para.scn_win.y = 0;
            var_to_disp_fb(&(layer_para.fb), &(info->var), &(info->fix));
            layer_para.fb.addr[0] = (__u32)info->fix.smem_start;
            layer_para.fb.addr[1] = 0;
            layer_para.fb.addr[2] = 0;
            layer_para.fb.size.width = fb_para->width;
            layer_para.fb.size.height = fb_para->height;
            layer_para.fb.cs_mode = DISP_BT601;
            layer_para.b_from_screen = 0;
            BSP_disp_layer_set_para(sel, hdl, &layer_para);

            BSP_disp_layer_open(sel, hdl);
            
        	g_fbi.layer_hdl[fb_id][sel] = hdl;
    	}
	}
    
    g_fbi.fb_enable[fb_id] = 1;
	g_fbi.fb_mode[fb_id] = fb_para->fb_mode;
    memcpy(&g_fbi.fb_para[fb_id], fb_para, sizeof(__disp_fb_create_para_t));
    
    return DIS_SUCCESS;
}

__s32 Display_Fb_Release(__u32 fb_id)
{
    __inf("Display_Fb_Release, fb_id:%d\n", fb_id);
    
	if((fb_id >= 0) && g_fbi.fb_enable[fb_id])
	{
        __u32 sel = 0;
        struct fb_info * info = g_fbi.fbinfo[fb_id];

        for(sel = 0; sel < 2; sel++)
        {
            if(((sel==0) && (g_fbi.fb_mode[fb_id] != FB_MODE_SCREEN1))
            || ((sel==1) && (g_fbi.fb_mode[fb_id] != FB_MODE_SCREEN0)))
            {
                __s32 layer_hdl = g_fbi.layer_hdl[fb_id][sel];
                
                BSP_disp_layer_release(sel, layer_hdl);
            }
        }
        g_fbi.layer_hdl[fb_id][0] = 0;
        g_fbi.layer_hdl[fb_id][1] = 0;
        g_fbi.fb_mode[fb_id] = FB_MODE_SCREEN0;
        memset(&g_fbi.fb_para[fb_id], 0, sizeof(__disp_fb_create_para_t));
        g_fbi.fb_enable[fb_id] = 0;
        
    	Fb_unmap_video_memory(info);

	    return DIS_SUCCESS;
	}
	else
	{
	    __wrn("invalid paras fb_id:%d in Display_Fb_Release\n", fb_id);
	    return DIS_FAIL;
	}
}

__s32 Display_Fb_get_para(__u32 fb_id, __disp_fb_create_para_t *fb_para)
{
    __inf("Display_Fb_Release, fb_id:%d\n", fb_id);
    
	if((fb_id >= 0) && g_fbi.fb_enable[fb_id])
	{
        memcpy(fb_para, &g_fbi.fb_para[fb_id], sizeof(__disp_fb_create_para_t));
        
	    return DIS_SUCCESS;
	}
	else
	{
	    __wrn("invalid paras fb_id:%d in Display_Fb_get_para\n", fb_id);
	    return DIS_FAIL;
	}
}

__s32 Display_get_disp_init_para(__disp_init_t * init_para)
{
    memcpy(init_para, &g_fbi.disp_init, sizeof(__disp_init_t));
    
    return 0;
}

__s32 Display_set_fb_timming(__u32 sel)
{
	__u8 fb_id=0;

	for(fb_id=0; fb_id<FB_MAX; fb_id++)
	{
		if(g_fbi.fb_enable[fb_id])
		{
	        if(((sel==0) && (g_fbi.fb_mode[fb_id] == FB_MODE_SCREEN0 || g_fbi.fb_mode[fb_id] == FB_MODE_DUAL_SAME_SCREEN_TB))
                || ((sel==1) && (g_fbi.fb_mode[fb_id] == FB_MODE_SCREEN1))
                || ((sel == g_fbi.fb_para[fb_id].primary_screen_id) && (g_fbi.fb_mode[fb_id] == FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS)))
            {
                __disp_tcon_timing_t tt;

                if(BSP_disp_get_timming(sel, &tt)>=0)
                {
                    g_fbi.fbinfo[fb_id]->var.pixclock = 1000000000 / tt.pixel_clk;
                    g_fbi.fbinfo[fb_id]->var.left_margin = tt.hor_back_porch;
                    g_fbi.fbinfo[fb_id]->var.right_margin = tt.hor_front_porch;
                    g_fbi.fbinfo[fb_id]->var.upper_margin = tt.ver_back_porch;
                    g_fbi.fbinfo[fb_id]->var.lower_margin = tt.ver_front_porch;
                    g_fbi.fbinfo[fb_id]->var.hsync_len = tt.hor_sync_time;
                    g_fbi.fbinfo[fb_id]->var.vsync_len = tt.ver_sync_time;
                }
            }
		}
	}

    return 0;
}

extern unsigned long fb_start;
extern unsigned long fb_size;
static int bootlogo_sz = 0;
int disp_get_parameter_for_cmdlind(char *cmdline, char *name, char *value)
{
    char *p = cmdline;
    char *value_p = value;

    if (!cmdline || !name) {
        return -1;
    }
    for (;;) {
        if (*p == ' ') {
            if (!strncmp(++p, name, sizeof(name))) {
                while (*p != '=' && *p)
                    p++;
                p++;
                while (*p != ' ' && *p) {
                    *value_p++ = *p++;
                }
                *value_p = 0;
                break;
            }
        }
        p++;
        if (!*p)
            break;
    }
    return 0;
}

static s32 fb_parse_bootlogo_base(phys_addr_t *fb_base, int * fb_size)
{
    char val[32];
    char *endp;

    memset(val, 0, sizeof(char) * 16);
    disp_get_parameter_for_cmdlind(saved_command_line, "fb_base", val);

    *fb_base = 0x0;
    *fb_base = memparse(val, &endp);
    printk(KERN_ERR "############base=%x\n",memparse(val, &endp));
    if (*endp == '@') {
        *fb_size = *fb_base;
        *fb_base = memparse(endp + 1, NULL);
    }

    return 0;
}
__s32 Fb_Init(__u32 from)
{    
    __disp_fb_create_para_t fb_para;
    __s32 i;
    __bool need_open_hdmi = 0;
	printk(KERN_ERR "[DISP]+++++Fb_Init++++++++\n");

	mutex_init(&gcommit_mutek);
 	INIT_WORK(&g_fbi.commit_work, hwc_commit_work);
    INIT_LIST_HEAD(&g_fbi.update_regs_list);
    g_fbi.timeline = sw_sync_timeline_create("sun5i-fb");
    g_fbi.timeline_max = 1;
    g_fbi.b_no_output = 0;
    mutex_init(&g_fbi.update_regs_list_lock);
    spin_lock_init(&(g_fbi.update_reg_lock));
	fb_parse_bootlogo_base(&bootlogo_addr, &bootlogo_sz);
    if(from == 0)//call from lcd driver
    {
#ifdef FB_RESERVED_MEM
#if 0
        __inf("fbmem: fb_start=%lu, fb_size=%lu\n", fb_start, fb_size);
        disp_create_heap((unsigned long)(__va(fb_start)),  fb_size);
#endif
#endif

        for(i=0; i<8; i++)
        {
		printk(KERN_ERR "$$$$$$$$$$$$$$$$$$$$\n");
        	g_fbi.fbinfo[i] = framebuffer_alloc(0, g_fbi.dev);
        	g_fbi.fbinfo[i]->fbops   = &dispfb_ops;
        	g_fbi.fbinfo[i]->flags   = 0; 
        	g_fbi.fbinfo[i]->device  = g_fbi.dev;
        	g_fbi.fbinfo[i]->par     = &g_fbi;
        	g_fbi.fbinfo[i]->var.xoffset         = 0;
        	g_fbi.fbinfo[i]->var.yoffset         = 0;
        	g_fbi.fbinfo[i]->var.xres            = 800;
        	g_fbi.fbinfo[i]->var.yres            = 480;
        	g_fbi.fbinfo[i]->var.xres_virtual    = 800;
        	g_fbi.fbinfo[i]->var.yres_virtual    = 480*2;
        	g_fbi.fbinfo[i]->var.nonstd = 0;
            g_fbi.fbinfo[i]->var.bits_per_pixel = 32;
            g_fbi.fbinfo[i]->var.transp.length = 8;
            g_fbi.fbinfo[i]->var.red.length = 8;
            g_fbi.fbinfo[i]->var.green.length = 8;
            g_fbi.fbinfo[i]->var.blue.length = 8;
            g_fbi.fbinfo[i]->var.transp.offset = 24;
            g_fbi.fbinfo[i]->var.red.offset = 16;
            g_fbi.fbinfo[i]->var.green.offset = 8;
            g_fbi.fbinfo[i]->var.blue.offset = 0;
            g_fbi.fbinfo[i]->var.activate = FB_ACTIVATE_FORCE;
        	g_fbi.fbinfo[i]->fix.type	    = FB_TYPE_PACKED_PIXELS;
        	g_fbi.fbinfo[i]->fix.type_aux	= 0;
        	g_fbi.fbinfo[i]->fix.visual 	= FB_VISUAL_TRUECOLOR;
        	g_fbi.fbinfo[i]->fix.xpanstep	= 1;
        	g_fbi.fbinfo[i]->fix.ypanstep	= 1;
        	g_fbi.fbinfo[i]->fix.ywrapstep	= 0;
        	g_fbi.fbinfo[i]->fix.accel	    = FB_ACCEL_NONE;
            g_fbi.fbinfo[i]->fix.line_length = g_fbi.fbinfo[i]->var.xres_virtual * 4;
            g_fbi.fbinfo[i]->fix.smem_len = g_fbi.fbinfo[i]->fix.line_length * g_fbi.fbinfo[i]->var.yres_virtual * 2;
            g_fbi.fbinfo[i]->screen_base = 0x0;
            g_fbi.fbinfo[i]->fix.smem_start = 0x0;

        	register_framebuffer(g_fbi.fbinfo[i]);
        }
        parser_disp_init_para(&(g_fbi.disp_init));
    }

    
    if(g_fbi.disp_init.b_init)
    {
        __u32 sel = 0;

        for(sel = 0; sel<2; sel++)
        {
            if(((sel==0) && (g_fbi.disp_init.disp_mode!=DISP_INIT_MODE_SCREEN1)) || 
                ((sel==1) && (g_fbi.disp_init.disp_mode!=DISP_INIT_MODE_SCREEN0)))
            {
                if(g_fbi.disp_init.output_type[sel] == DISP_OUTPUT_TYPE_HDMI)
                {
                    need_open_hdmi = 1;
                }
            }
        }
    }

    if(need_open_hdmi == 1 && from == 0)//it is called from lcd driver, but hdmi need to be opened
    {
        return 0;
    }
    else if(need_open_hdmi == 0 && from == 1)//it is called from hdmi driver, but hdmi need not be opened
    {
        return 0;
    }
    
    if(g_fbi.disp_init.b_init)
    {
        __u32 fb_num = 0, sel = 0;

        for(sel = 0; sel<2; sel++)
        {
            if(((sel==0) && (g_fbi.disp_init.disp_mode!=DISP_INIT_MODE_SCREEN1)) || 
                ((sel==1) && (g_fbi.disp_init.disp_mode!=DISP_INIT_MODE_SCREEN0)))
            {
                if(g_fbi.disp_init.output_type[sel] == DISP_OUTPUT_TYPE_LCD)
                {
                    DRV_lcd_open(sel);
                }
                else if(g_fbi.disp_init.output_type[sel] == DISP_OUTPUT_TYPE_TV)
                {
                    BSP_disp_tv_set_mode(sel, g_fbi.disp_init.tv_mode[sel]);
                    BSP_disp_tv_open(sel);
                }
                 else if(g_fbi.disp_init.output_type[sel] == DISP_OUTPUT_TYPE_HDMI)
                {
                    BSP_disp_hdmi_set_mode(sel, g_fbi.disp_init.tv_mode[sel]);
                    BSP_disp_hdmi_open(sel);
                }
                 else if(g_fbi.disp_init.output_type[sel] == DISP_OUTPUT_TYPE_VGA)
                {
                    BSP_disp_vga_set_mode(sel, g_fbi.disp_init.vga_mode[sel]);
                    BSP_disp_vga_open(sel);
                }
            }
        }

        fb_num = (g_fbi.disp_init.disp_mode==DISP_INIT_MODE_TWO_DIFF_SCREEN)?2:1;
        for(i = 0; i<fb_num; i++)
        {
            __u32 screen_id = i;

            disp_fb_to_var(g_fbi.disp_init.format[i], g_fbi.disp_init.seq[i], g_fbi.disp_init.br_swap[i], &(g_fbi.fbinfo[i]->var));
            
            if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN1)
            {
                screen_id = 1;
            }
            fb_para.buffer_num= g_fbi.disp_init.buffer_num[i];
            fb_para.width = BSP_disp_get_screen_width(screen_id);
            fb_para.height = BSP_disp_get_screen_height(screen_id);
            fb_para.output_width = BSP_disp_get_screen_width(screen_id);
            fb_para.output_height = BSP_disp_get_screen_height(screen_id);
            fb_para.mode = (g_fbi.disp_init.scaler_mode[i]==0)?DISP_LAYER_WORK_MODE_NORMAL:DISP_LAYER_WORK_MODE_SCALER;
            if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN0)
            {
                fb_para.fb_mode = FB_MODE_SCREEN0;
            }
            else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN1)
            {
                fb_para.fb_mode = FB_MODE_SCREEN1;
            }
            else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_TWO_DIFF_SCREEN)
            {
                if(i == 0)
                {
                    fb_para.fb_mode = FB_MODE_SCREEN0;
                }
                else
                {
                    fb_para.fb_mode = FB_MODE_SCREEN1;
                }
            }
            else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_TWO_SAME_SCREEN)
            {
                fb_para.fb_mode = FB_MODE_DUAL_SAME_SCREEN_TB;
                fb_para.height *= 2;
                fb_para.output_height *= 2;
            }
            else if(g_fbi.disp_init.disp_mode == DISP_INIT_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS)
            {
                fb_para.fb_mode = FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS;
                fb_para.output_width = BSP_disp_get_screen_width(fb_para.primary_screen_id);
                fb_para.output_height = BSP_disp_get_screen_height(fb_para.primary_screen_id);
                fb_para.aux_output_width = BSP_disp_get_screen_width(1 - fb_para.primary_screen_id);
                fb_para.aux_output_height = BSP_disp_get_screen_height(1 - fb_para.primary_screen_id);
            }
            Display_Fb_Request(i, &fb_para);
#if 0
            fb_draw_colorbar((__u32)g_fbi.fbinfo[i]->screen_base, fb_para.width, fb_para.height*fb_para.buffer_num, &(g_fbi.fbinfo[i]->var));
#endif
        }
#if 0
        if(g_fbi.disp_init.scaler_mode[0])
        {
            BSP_disp_print_reg(0, DISP_REG_SCALER0);
        }
        if(g_fbi.disp_init.scaler_mode[1])
        {
            BSP_disp_print_reg(0, DISP_REG_SCALER1);
        }
    	if(g_fbi.disp_init.disp_mode != DISP_INIT_MODE_SCREEN1)
    	{
            BSP_disp_print_reg(0, DISP_REG_IMAGE0);
            BSP_disp_print_reg(0, DISP_REG_LCDC0);
            if((g_fbi.disp_init.output_type[0] == DISP_OUTPUT_TYPE_TV) || (g_fbi.disp_init.output_type[0] == DISP_OUTPUT_TYPE_VGA))
            {
                BSP_disp_print_reg(0, DISP_REG_TVEC0);
            }
        }
        if(g_fbi.disp_init.disp_mode != DISP_INIT_MODE_SCREEN0)
        {
    	    BSP_disp_print_reg(0, DISP_REG_IMAGE1);
    	    BSP_disp_print_reg(0, DISP_REG_LCDC1); 
    	    if((g_fbi.disp_init.output_type[1] == DISP_OUTPUT_TYPE_TV) || (g_fbi.disp_init.output_type[1] == DISP_OUTPUT_TYPE_VGA))
            {
                BSP_disp_print_reg(0, DISP_REG_TVEC1);
            }
        }
        BSP_disp_print_reg(0, DISP_REG_CCMU); 
        BSP_disp_print_reg(0, DISP_REG_PWM);
        BSP_disp_print_reg(0, DISP_REG_PIOC); 
		#endif
    }

    INIT_WORK(&g_fbi.vsync_work[0], send_vsync_work_0);
    INIT_WORK(&g_fbi.vsync_work[1], send_vsync_work_1);
    
	return 0;
}

__s32 Fb_Exit(void)
{
	__u8 fb_id=0;

	for(fb_id=0; fb_id<FB_MAX; fb_id++)
	{
		if(g_fbi.fbinfo[fb_id] != NULL)
		{
			Display_Fb_Release(FBIDTOHAND(fb_id));
		}
	}

	for(fb_id=0; fb_id<8; fb_id++)
	{
    	unregister_framebuffer(g_fbi.fbinfo[fb_id]);
    	framebuffer_release(g_fbi.fbinfo[fb_id]);
    	g_fbi.fbinfo[fb_id] = NULL;
	}
	
	return 0;
}

EXPORT_SYMBOL(Fb_Init);

