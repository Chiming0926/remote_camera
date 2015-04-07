#ifndef __CAMERA__H_
#define __CAMERA__H_


#define DEVICE_NAME_LEN 128

struct camera_ops;

typedef struct
{  
    void *start;  
    size_t length;  
} video_buffer;

typedef struct 
{
	int 	fd;
	char 	dev_name[DEVICE_NAME_LEN];
	int		video_buffer_num; 
	struct camera_ops *cam_ops;
	video_buffer	   *video_buf;
} cam_info;

struct camera_ops
{	
	int (*cam_open)(cam_info *info);
	int (*cam_release)(cam_info *info);
};
cam_info *create_camera_module(char *dev_name);
#endif
