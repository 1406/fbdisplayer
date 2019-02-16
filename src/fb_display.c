#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/fb.h>

#include <sys/mman.h>

#include <string.h>

struct fb_info
{
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;

    int fd;
    void *buff;
    unsigned long long bufflen;
    unsigned int cpp;
};

struct img_info
{
    int x, y;
    int width, height;
    void *data;
};

int fb_open(const char *name, struct fb_info *fb)
{
    fb->fd = open(name, O_RDWR);
    if (fb->fd < 0)
    {
        return fb->fd;
    }

    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, fb->fix) < 0)
    {
        close(fb->fd);
        return -1;
    }

    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, fb->var) < 0)
    {
        close(fb->fd);
        return -1;
    }

    fb->cpp = fb->var.bits_per_pixel >> 3;
    fb->bufflen = fb->var.xres * fb->var.yres * fb->cpp;
    fb->buff = mmap(NULL, fb->bufflen, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
    if (fb->buff == MAP_FAILED)
    {
        close(fb->fd);
        return -1;
    }

    return 0;
}

int fb_close(struct fb_info *fb)
{
    int ret = 0;

    if (munmap(fb->fd, fb->bufflen))
    {
        ret = -1;
    }

    if (close(fb->fd))
    {
        ret = -1;
    }

    return ret;
}

int fb_clean(struct fb_info *fb)
{
    memset(fb->buff, 0, fb->bufflen);
}

int fb_display(struct fb_info *fb, struct img_info *img)
{
    int linelen = fb->var.xres - img->x >= img->width ? img->width : fb->var.xres - img->x;
    if (linelen <= 0)
    {
        return 1;
    }

    int linecnt = fb->var.yres - img->y >= img->height ? img->height : fb->var.yres - img->y;
    if (linecnt <= 0)
    {
        return 1;
    }

    int i;
    unsigned char *fbbuff = (unsigned char *)fb->buff + img->x; /* add the offset once but offset per line */
    unsigned char *imgdata = (unsigned char *)img->data;

    for (i = img->y; i < linecnt; i++, fbbuff += fb->var.xres * fb->cpp, imgdata += img->width * fb->cpp)
    {
        memcpy(fbbuff, imgdata, linelen);
    }

    return 0;
}
