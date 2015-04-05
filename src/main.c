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
#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {  
    void * start;  
    size_t length;  
};  
  
struct buffer * buffers = NULL;  
static unsigned int n_buffers = 0;  

static void init_mmap(char *dev_name, int fd) 
{  
    struct v4l2_requestbuffers req;  
  
    CLEAR(req);  
  
    req.count = 4;  
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    req.memory = V4L2_MEMORY_MMAP;  
  
    if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) 
	{  
        if (EINVAL == errno) 
		{  
            fprintf(stderr, "%s does not support "  
                    "memory mapping/n", dev_name);  
            exit(EXIT_FAILURE);  
        } 
		else 
		{  
			exit(1);
            //errno_exit("VIDIOC_REQBUFS");  
        }  
    }  
  
    if (req.count < 2) 
	{  
        fprintf(stderr, "Insufficient buffer memory on %s/n", dev_name);  
        exit(EXIT_FAILURE);  
    }  
  
    buffers = calloc(req.count, sizeof(*buffers));  
  
    if (!buffers) {  
        fprintf(stderr, "Out of memory/n");  
        exit(EXIT_FAILURE);  
    }  
  
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) 
	{  
        struct v4l2_buffer buf;  
  
        CLEAR(buf);  
  
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory = V4L2_MEMORY_MMAP;  
        buf.index = n_buffers;  
  
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))  
            exit(1);//errno_exit("VIDIOC_QUERYBUF");  
  
        buffers[n_buffers].length = buf.length;  
        buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,  
                PROT_READ | PROT_WRITE /* required */,  
                MAP_SHARED /* recommended */, fd, buf.m.offset);  
  
        if (MAP_FAILED == buffers[n_buffers].start)  
            exit(1);//errno_exit("mmap");  
    }  
} 

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

static void init_device(char *dev_name, int fd) 
{  
    struct v4l2_capability cap;  
    struct v4l2_cropcap cropcap;  
    struct v4l2_crop crop;  
    struct v4l2_format fmt, fmt1;  
    unsigned int min;  
 
    if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap)) 
	{  
        if (EINVAL == errno) 
		{  
            fprintf(stderr, "%s is no V4L2 device/n", dev_name);  
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
        fprintf(stderr, "%s is no video capture device/n", dev_name);  
        exit(EXIT_FAILURE);  
    }  

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) 
	{  
    	fprintf(stderr, "%s does not support streaming i/o/n", dev_name);  
    	exit(EXIT_FAILURE);  
   	}  
  
  
    /* Select video input, video standard and tune here. */  
  
    CLEAR(cropcap);  

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
  
    if (0 == ioctl(fd, VIDIOC_CROPCAP, &cropcap)) 
	{  
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        crop.c = cropcap.defrect; /* reset to default */  
  
        if (-1 == ioctl(fd, VIDIOC_S_CROP, &crop)) 
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
	ioctl(fd, VIDIOC_G_FMT, &fmt1);


    CLEAR(fmt);  
  
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	fmt.fmt.pix.width = fmt1.fmt.pix.width;
	fmt.fmt.pix.height = fmt1.fmt.pix.height;
	fmt.fmt.pix.pixelformat = fmt1.fmt.pix.pixelformat;
    if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt))  
        exit(1);//errno_exit("VIDIOC_S_FMT");  

    /* Note VIDIOC_S_FMT may change width and height. */  
  
    /* Buggy driver paranoia. */  
    min = fmt.fmt.pix.width * 2;  
    if (fmt.fmt.pix.bytesperline < min)  
        fmt.fmt.pix.bytesperline = min;  
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;  
    if (fmt.fmt.pix.sizeimage < min)  
        fmt.fmt.pix.sizeimage = min;  

    init_mmap(dev_name, fd);  

}  
static void close_device(int *fd) 
{  
    if (-1 == close(*fd))  
 	{
		printf("close function failed \n");
		return;
	}
    *fd = -1;  
} 

static void open_device(char *dev_name, int *fd)
{
  	struct stat st;  
    if (-1 == stat(dev_name, &st)) 
	{  
        fprintf(stderr, "Cannot identify '%s': %d, %s/n", dev_name, errno,  
                strerror(errno));  
        exit(EXIT_FAILURE);  
    }  
  
    if (!S_ISCHR(st.st_mode)) 
	{  
        fprintf(stderr, "%s is no device/n", dev_name);  
        exit(EXIT_FAILURE);  
    }  
  
    *fd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);  
  
    if (-1 == *fd) 
	{  
        fprintf(stderr, "Cannot open '%s': %d, %s/n", dev_name, errno,  
                strerror(errno));  
        exit(EXIT_FAILURE);  
    }
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
	printf("Hello webcam \n");
	char *dev_name = "/dev/video0";  
  	int fd = -1;


	open_device(dev_name, &fd);

	init_device(dev_name, fd);

	start_capturing(fd);
	mainloop(fd);

	printf("881 webcam\n");
	close_device(&fd);
}
