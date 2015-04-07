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

static void init_mmap(cam_info *info) 
{  
	int i=0;
    struct v4l2_requestbuffers req;  
    memset(&req, 0, sizeof(req));  
  
    req.count = 4;  
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    req.memory = V4L2_MEMORY_MMAP;  
 
    if (-1 == ioctl(info->fd, VIDIOC_REQBUFS, &req)) 
	{  
        if (EINVAL == errno) 
		{  
            fprintf(stderr, "%s does not support "  
                    "memory mapping/n", info->dev_name);  
            exit(EXIT_FAILURE);  
        } 
		else 
		{  
			exit(EXIT_FAILURE);
        }  
    }  
  
    if (req.count < 2) 
	{  
        fprintf(stderr, "Insufficient buffer memory on %s/n", info->dev_name);  
        exit(EXIT_FAILURE);  
    }  

	info->video_buffer_num = req.count;
	info->video_buf = (video_buffer*)malloc(sizeof(video_buffer)*info->video_buffer_num);
  
    if (!info->video_buf) 
	{  
        fprintf(stderr, "Out of memory/n");  
        exit(EXIT_FAILURE);  
    }  
  
    for (i = 0; i < info->video_buffer_num; i++) 
	{  
        struct v4l2_buffer buf;  
  
        memset(&buf, 0, sizeof(buf)); 
  
        buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory 	= V4L2_MEMORY_MMAP;  
        buf.index 	= i;  
  
        if (-1 == ioctl(info->fd, VIDIOC_QUERYBUF, &buf))  
            exit(EXIT_FAILURE);  
  
        info->video_buf[i].length = buf.length;  
        info->video_buf[i].start = mmap(NULL, buf.length,  
                PROT_READ | PROT_WRITE ,  
                MAP_SHARED, info->fd, buf.m.offset);  
  
        if (MAP_FAILED == info->video_buf[i].start)  
            exit(EXIT_FAILURE);
    }
} 

static void init_device(cam_info *info) 
{  
    struct v4l2_capability cap;  
    struct v4l2_cropcap cropcap;  
    struct v4l2_crop crop;  
    struct v4l2_format fmt, fmt1;  
    unsigned int min;  
 
    if (-1 == ioctl(info->fd, VIDIOC_QUERYCAP, &cap)) 
	{  
        if (EINVAL == errno) 
		{  
            fprintf(stderr, "%s is no V4L2 device/n", info->dev_name);  
            exit(EXIT_FAILURE);  
        } 
		else 
		{
			return;
        }  
    }

	printf("driver 	= %s \n", cap.driver);
	printf("card 	= %s \n", cap.card);
	printf("bus		= %s \n", cap.bus_info);
	printf("version = %d \n", cap.version);
	printf("capabilities = %d \n", cap.capabilities);


    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) 
	{  
        fprintf(stderr, "%s is no video capture device/n", info->dev_name);  
        exit(EXIT_FAILURE);  
    }  

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) 
	{  
    	fprintf(stderr, "%s does not support streaming i/o/n", info->dev_name);  
    	exit(EXIT_FAILURE);  
   	}  
  
  
    /* Select video input, video standard and tune here. */  
	memset(&cropcap, 0, sizeof(cropcap));	
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
  
    if (0 == ioctl(info->fd, VIDIOC_CROPCAP, &cropcap)) 
	{  
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        crop.c = cropcap.defrect; /* reset to default */  
  
        if (-1 == ioctl(info->fd, VIDIOC_S_CROP, &crop)) 
		{  
            switch (errno) 
			{  
            case EINVAL:  
                /* Cropping not supported. */  
                break;  
            default:  
                /* Errors ignored. */  
                break;  
            }  
        }  
    } 
	else 
	{  
        /* Errors ignored. */  
    }  


	fmt1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(info->fd, VIDIOC_G_FMT, &fmt1);

	memset(&fmt, 0, sizeof(fmt));	
  
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	fmt.fmt.pix.width = fmt1.fmt.pix.width;
	fmt.fmt.pix.height = fmt1.fmt.pix.height;
	fmt.fmt.pix.pixelformat = fmt1.fmt.pix.pixelformat;
    if (-1 == ioctl(info->fd, VIDIOC_S_FMT, &fmt))  
        exit(EXIT_FAILURE);  

    /* Note VIDIOC_S_FMT may change width and height. */  
  
    /* Buggy driver paranoia. */  
    min = fmt.fmt.pix.width * 2;  
    if (fmt.fmt.pix.bytesperline < min)  
        fmt.fmt.pix.bytesperline = min;  
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;  
    if (fmt.fmt.pix.sizeimage < min)  
        fmt.fmt.pix.sizeimage = min;  

    init_mmap(info);  

}  
 
static int cam_open(cam_info *info)
{
  	struct stat st; 
    if (-1 == stat(info->dev_name, &st)) 
	{  
        fprintf(stderr, "Cannot identify '%s': %d, %s/n", info->dev_name, errno,  
                strerror(errno));
		return -1;
	}

    if (!S_ISCHR(st.st_mode)) 
	{  
        fprintf(stderr, "%s is no device/n", info->dev_name);  
        return -1;  
    }  
  
    info->fd = open(info->dev_name, O_RDWR | O_NONBLOCK, 0);  
  
    if (-1 == info->fd) 
	{  
        fprintf(stderr, "Cannot open '%s': %d, %s/n", info->dev_name, errno,  
		strerror(errno));
		return -1;
	}
	init_device(info);
	return 0;
}

static int cam_close(cam_info *info)
{
	if (info == NULL)
		return -1;
	if (info->fd > 0)
	{
		close(info->fd);
		free(info);
	}
	return 0;
}

static struct camera_ops cam_ops = 
{
	.cam_open = cam_open,
	.cam_release = cam_close,
};

cam_info *create_camera_module(char *dev_name)
{
	if (dev_name)
	{
		cam_info *info;
		info = (cam_info*)malloc(sizeof(cam_info));
		if (info)
		{
			int len = strlen(dev_name) > DEVICE_NAME_LEN 
				? DEVICE_NAME_LEN : strlen(dev_name);
			strncpy(info->dev_name, dev_name, len);
			info->cam_ops = &cam_ops;
			return info;
		}
	}
	return NULL;
}

