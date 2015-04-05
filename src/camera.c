#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <assert.h>  
#include <getopt.h>              
#include <fcntl.h>                
#include <unistd.h>  
#include <errno.h>  
#include <malloc.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <sys/time.h>  
#include <sys/mman.h>  
#include <sys/ioctl.h>  
#include <asm/types.h>            
#include <linux/videodev2.h> 
#include <camera.h>
 
static int cam_open(cam_info *info, char *dev_name)
{
  	struct stat st; 
    if (-1 == stat(dev_name, &st)) 
	{  
        fprintf(stderr, "Cannot identify '%s': %d, %s/n", dev_name, errno,  
                strerror(errno));
		return -1;
	}

    if (!S_ISCHR(st.st_mode)) 
	{  
        fprintf(stderr, "%s is no device/n", dev_name);  
        return -1;  
    }  
  
    info->fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);  
  
    if (-1 == info->fd) 
	{  
        fprintf(stderr, "Cannot open '%s': %d, %s/n", dev_name, errno,  
		strerror(errno));
		return -1;
	}
	return 0;
}

static int cam_close(cam_info *info)
{
	return 0;
}

static camera_ops cam_ops = 
{
	.cam_open = cam_open,
	.cam_release = cam_close,
};

