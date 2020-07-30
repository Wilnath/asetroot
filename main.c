#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <Imlib2.h>

Display            *XDPY;
unsigned int        XSCRN_NUM;
Screen             *XSCRN;
Window              ROOT_WIN;
int                 BITDEPTH;
Colormap            COLORMAP;
Visual             *VISUAL;

int NUMBER_OF_FRAMES;

Atom prop_root, prop_esetroot;

// Since these seem to never change throughout X's lifetime we can just run this once
// keyword there, seem, this might break something I'm not aware of
void set_atoms() {
	prop_root = XInternAtom(XDPY, "_XROOTPMAP_ID", False);
	prop_esetroot = XInternAtom(XDPY, "ESETROOTPMAP_ID", False);

    if (prop_root == None || prop_esetroot == None)
	{
		printf("Failed to get _XROOTPMAP_ID or ESETROOTPMAP_ID atom\n");
		exit(1);
	}
}

void one_time_pixmap_setup(Pixmap p)
{
    XChangeProperty(XDPY, ROOT_WIN, prop_esetroot, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *) &p, 1);
}

// From setroot, sets a pixmap to be root
// Commented a bunch of stuff out that doesn't seem to be needed
void
set_pixmap_property(Pixmap p)
{
//  Atom prop_root, prop_esetroot;
//	Atom type;
//  int format;
//  unsigned long length, after;
//  unsigned char *data_root, *data_esetroot;

//    if ((prop_root != None) && (prop_esetroot != None)) {
//        XGetWindowProperty(XDPY, ROOT_WIN, prop_root, 0L, 1L, False,
//                           AnyPropertyType, &type, &format,
//                           &length, &after, &data_root);
//
//        if (type == XA_PIXMAP) {
//            XGetWindowProperty(XDPY, ROOT_WIN, prop_esetroot, 0L, 1L,
//                               False, AnyPropertyType, &type, &format,
//                               &length, &after, &data_esetroot);
//
//            if (data_esetroot) { free(data_esetroot); }
//        }
//        if (data_root) { free(data_root); }
//    }

//    prop_root = XInternAtom(XDPY, "_XROOTPMAP_ID", False);
//    prop_esetroot = XInternAtom(XDPY, "ESETROOTPMAP_ID", False);

    XChangeProperty(XDPY, ROOT_WIN, prop_root, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *) &p, 1);
//    XChangeProperty(XDPY, ROOT_WIN, prop_esetroot, XA_PIXMAP, 32,
//                    PropModeReplace, (unsigned char *) &p, 1);

    XSetCloseDownMode(XDPY, RetainPermanent);
    XFlush(XDPY);
}

Imlib_Image *load_images_from_folder_formatted(const char* folder_path, const char* format) {
	int frame_number = 0;
	const char* extension = ".gif";
	char current_file[128];
	char file_name[128];
	snprintf(file_name, 128, format, frame_number);
	memset(current_file, 0, sizeof(current_file));
	strcat(current_file, folder_path);
	strcat(current_file, file_name);
	strcat(current_file, extension);
	Imlib_Image *images = NULL;
	while (access(current_file, R_OK) != -1){
		Imlib_Image temp_image;
		temp_image = imlib_load_image(current_file);
		frame_number++;
		images = realloc(images, frame_number*sizeof(char*));
		*(images+frame_number-1) = temp_image;
		memset(current_file, 0, sizeof(current_file));
		snprintf(file_name, 128, format, frame_number);
		strcat(current_file, folder_path);
		strcat(current_file, file_name);
		strcat(current_file, extension);
	}
	NUMBER_OF_FRAMES = frame_number;
	return images;
}

Pixmap *load_pixmaps_from_images(const Imlib_Image *images, const int amount)
{
	Pixmap *temp = malloc(sizeof(Pixmap)*amount);
	for (int i = 0; i < amount; i++) {
		*(temp+i) = XCreatePixmap(XDPY, ROOT_WIN, 1920, 1080, BITDEPTH);
		imlib_context_set_drawable(*(temp+i));
		imlib_context_set_image(*(images+i));
		imlib_render_image_on_drawable(0, 0);
	}
	return temp;
}

struct timespec timespec_get_difference(struct timespec start, struct timespec end)
{
    struct timespec temp;

    if ((end.tv_nsec-start.tv_nsec)<0)
    {
            temp.tv_sec = end.tv_sec-start.tv_sec-1;
            temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    }
    else 
    {
            temp.tv_sec = end.tv_sec-start.tv_sec;
            temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}
int main(int argc, const char *argv[])
{
	if (argc < 2) {
		printf("Usage: asetroot <folder>\n");
		printf("\n");
		printf("Folder contains frames with the names with the format %%05d in order.\n");
		exit(1);
	}

	struct timespec start={0,0}, end={0,0};
	int nsec_per_frame = 75*1000; // *1000 to turn msec into nsec

	XDPY = XOpenDisplay(NULL);
    XSCRN_NUM = DefaultScreen(XDPY);
    XSCRN = ScreenOfDisplay(XDPY, XSCRN_NUM);
    ROOT_WIN = RootWindow(XDPY, XSCRN_NUM);
    COLORMAP = DefaultColormap(XDPY, XSCRN_NUM);
    VISUAL = DefaultVisual(XDPY, XSCRN_NUM);
    BITDEPTH = DefaultDepth(XDPY, XSCRN_NUM);

	Imlib_Image *entries = load_images_from_folder_formatted(argv[1], "%05d");
	if (!entries) {
		printf("Failed getting images\n");
		exit(1);
	}

	imlib_context_set_display(XDPY);
	imlib_context_set_visual(VISUAL);
	imlib_context_set_colormap(COLORMAP);
	Pixmap *pixmaps = load_pixmaps_from_images(entries, NUMBER_OF_FRAMES);
	if (!pixmaps) {
		printf("Failed to initialize pixmaps\n");
		exit(1);
	}

	for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
		imlib_context_set_image(entries[i]);
		imlib_free_image();
	}

	int current = 0;
	int difference;
	clock_gettime(CLOCK_REALTIME, &start);
	set_atoms();
	Pixmap current_pixmap;
	while (1) {
		current_pixmap = *(pixmaps+current);
		set_pixmap_property(current_pixmap);
		current++;
		if (current >= NUMBER_OF_FRAMES) {
			current = 0;
		}
		clock_gettime(CLOCK_REALTIME, &end);
		difference = timespec_get_difference(start, end).tv_nsec/1000;
		if (difference > nsec_per_frame) {
			difference = nsec_per_frame;
		}
		usleep(nsec_per_frame-difference);
		clock_gettime(CLOCK_REALTIME, &start);
	}


	for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
		XFreePixmap(XDPY, *(pixmaps+i));
	}

	free(entries);

	return 0;
}
