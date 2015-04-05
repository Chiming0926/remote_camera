#ifndef __CAMERA__H_
#define __CAMERA__H_

typedef struct 
{
	int fd;
}cam_info;

typedef struct 
{	
	int (*cam_open)(cam_info *info, char *dev_name);
	int (*cam_release)(cam_info *info);
}camera_ops;

#endif
