#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <assert.h>  
  
#include <getopt.h>             /* getopt_long() */  
  
#include <fcntl.h>              /* low-level i/o */  
#include <unistd.h>  
#include <errno.h>  
#include <malloc.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <sys/time.h>  
#include <sys/mman.h>  
#include <sys/ioctl.h>  
  
#include <asm/types.h>          /* for videodev2.h */  
  
#include <linux/videodev2.h>  

#include <camera.h>
#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {  
    void * start;  
    size_t length;  
};  
  
struct buffer * buffers = NULL;  
static unsigned int n_buffers = 0;  



static void start_capturing(int fd) 
{

    unsigned int i;  
    enum v4l2_buf_type type;  
  
  
    for (i = 0; i < n_buffers; ++i) 
    { 

	    struct v4l2_buffer buf;  
  
        CLEAR(buf);  
  		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
   		buf.memory = V4L2_MEMORY_MMAP;  
		buf.index = i;  

		if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))  
			exit(1);//errno_exit("VIDIOC_QBUF");  
	}  

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

	if (-1 == ioctl(fd, VIDIOC_STREAMON, &type))  
		exit(1);//errno_exit("VIDIOC_STREAMON");  

}  


static int read_frame(int fd) 
{  
    struct v4l2_buffer buf;  
  
	CLEAR(buf);  

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
	buf.memory = V4L2_MEMORY_MMAP;  

	if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) 
	{  
		switch (errno) 
		{  
		case EAGAIN:  
			return 0;  

		case EIO:  
			/* Could ignore EIO, see spec. */  

			/* fall through */  

		default:  
			exit(1);//errno_exit("VIDIOC_DQBUF");  
		}  
	}  
	
	
	printf("Time = %ld.%06ld, index = %d\n", buf.timestamp.tv_sec, buf.timestamp.tv_usec,
			buf.index);
	//assert(buf.index < n_buffers);  

	//process_image(buffers[buf.index].start, buf.length);  

	if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))  
		exit(1);//errno_exit("VIDIOC_QBUF");  
    return 1;  
}  

static void mainloop(int fd)
{  
    unsigned int count;  
  
    count = 100;  
 	
	printf("@@@@@@@@@@@@@@ %s %s %d fd = %d\n", __FILE__, __FUNCTION__, __LINE__, fd);
    while (count-- > 0) 
	{  
        for (;;) 
		{  
            fd_set fds;  
            struct timeval tv;  
            int r;  
  
            FD_ZERO(&fds);  
            FD_SET(fd, &fds);  
  
            /* Timeout. */  
            tv.tv_sec = 2;  
            tv.tv_usec = 0;  
  
            r = select(fd + 1, &fds, NULL, NULL, &tv);  
  
            if (-1 == r) 
			{  
                if (EINTR == errno)  
                    continue;  
  				exit(1);	
                //errno_exit("select");  
            }  
  
            if (0 == r) 
			{  
                fprintf(stderr, "select timeout/n");  
//                exit(EXIT_FAILURE);  
            }  
  
            if (read_frame(fd))  
                break;  
  
            /* EAGAIN - continue select loop. */  
        }  
    }  
}  

int main(int argc, char *argv[])
{
	printf("Hello Remote Camera \n");
	char *dev_name = "/dev/video0";  
  	int fd = -1;
	cam_info *cinfo = NULL;
	cinfo = create_camera_module(dev_name);
	if (cinfo == NULL)
		return EXIT_FAILURE;
	cinfo->cam_ops->cam_open(cinfo);
/*	open_device(dev_name, &fd);

	init_device(dev_name, fd);

	start_capturing(fd);
	mainloop(fd);

	printf("881 webcam\n");
	close_device(&fd);*/
}
