#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <Imlib2.h>

/* GLOBALS */

Display            *XDPY;
unsigned int        XSCRN_NUM;
Screen             *XSCRN;
Window              ROOT_WIN;
int                 BITDEPTH;
Colormap            COLORMAP;
Visual             *VISUAL;
int NUMBER_OF_FRAMES;
Atom prop_root, prop_esetroot;
int keep_looping;

/* ARGS */

char FOLDER_NAME[256];
int MILLISECONDS_PER_FRAME = -1;
char FRAME_FILE_NAME_FORMAT[256];



void print_usage() {
	printf("Usage: asetroot [FOLDER] ... [-t milliseconds]\n");
	printf("Where [FOLDER] is a folder with all the frames.\n");
	printf("Frames should have a file name that can be incremented using a printf format, standard is %%05d.gif\n");
	printf("This program does not do any resizing or gif converting, if you wish to do that check out ImageMagick\n");
	printf("\n");
	printf("Options and arguments:\n");
	printf("-t    : sets the amount of milliseconds between each frame\n");
	printf("        default is 100\n");
	printf("-f    : sets the format for the file names in [FOLDER]\n");
	printf("        default is %%05d.gif\n");
}

void parse_args(const int argc, const char *argv[]) {
	int opt;
	strcat(FOLDER_NAME, argv[1]);
	// TODO : This throws an annoying warning right now
	while((opt = getopt(argc, argv, "t:f:h")) != -1) {
		switch(opt) {
			// TODO : This will probably end up bloating the main file after a while
			// consider moving parse_args into another file
			case 't':
				MILLISECONDS_PER_FRAME = atoi(optarg);
				if (MILLISECONDS_PER_FRAME < 1) {
					printf("ERROR: -t is an illegal value, %d\n", MILLISECONDS_PER_FRAME);
					print_usage();
					exit(1);
				}
				break;
			case 'f':
				strcat(FRAME_FILE_NAME_FORMAT, optarg);
				break;
			case 'h':
				print_usage();
				exit(1);
			case '?':
				print_usage();
				exit(1);
				break;
		}
	}
	if (MILLISECONDS_PER_FRAME == -1) {
		MILLISECONDS_PER_FRAME = 100;
	}
	if (FRAME_FILE_NAME_FORMAT[0] == '\0') {
		strcat(FRAME_FILE_NAME_FORMAT, "%05d.gif");
	}
}

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

// From setroot.c, sets a pixmap to be root
// Commented out some stuff that is handled elsewhere
// TODO : I'm unsure if the nested if cases are needed,
// However I'm scared that deleting them will cause some memory leak within X
void
set_pixmap_property(Pixmap p)
{
//  Atom prop_root, prop_esetroot;
	Atom type;
    int format;
    unsigned long length, after;
    unsigned char *data_root, *data_esetroot;

    if ((prop_root != None) && (prop_esetroot != None)) {
        XGetWindowProperty(XDPY, ROOT_WIN, prop_root, 0L, 1L, False,
                           AnyPropertyType, &type, &format,
                           &length, &after, &data_root);

        if (type == XA_PIXMAP) {
            XGetWindowProperty(XDPY, ROOT_WIN, prop_esetroot, 0L, 1L,
                               False, AnyPropertyType, &type, &format,
                               &length, &after, &data_esetroot);

            if (data_esetroot) { free(data_esetroot); }
        }
        if (data_root) { free(data_root); }
    }

//  prop_root = XInternAtom(XDPY, "_XROOTPMAP_ID", False);
//  prop_esetroot = XInternAtom(XDPY, "ESETROOTPMAP_ID", False);

    XChangeProperty(XDPY, ROOT_WIN, prop_root, XA_PIXMAP, 32,
                    PropModeReplace, (unsigned char *) &p, 1);
//    XChangeProperty(XDPY, ROOT_WIN, prop_esetroot, XA_PIXMAP, 32,
//                    PropModeReplace, (unsigned char *) &p, 1);

//    XSetCloseDownMode(XDPY, RetainPermanent);
    XFlush(XDPY);
}

Imlib_Image *load_images_from_folder_formatted(const char* folder_path, const char* format) {
	int frame_number = 0;
	char current_file[256];
	char file_name[256];
	snprintf(file_name, 128, format, frame_number);
	if (file_name[0] == '\0') {
		fprintf(stderr, "Error formatting file names using format %s\n", format);
		exit(1);
	}
	memset(current_file, 0, sizeof(current_file));
	strcat(current_file, folder_path);
	strcat(current_file, file_name);
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
	}
	NUMBER_OF_FRAMES = frame_number;
	return images;
}

Pixmap *load_pixmaps_from_images(const Imlib_Image *images, const int amount)
{
	int width = WidthOfScreen(XSCRN);
	int height = HeightOfScreen(XSCRN);
	Pixmap *temp = malloc(sizeof(Pixmap)*amount);
	for (int i = 0; i < amount; i++) {
		*(temp+i) = XCreatePixmap(XDPY, ROOT_WIN, width, height, BITDEPTH);
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

void exit_loop(int sig)
{
	printf("Received %s. Exiting...\n", strsignal(sig));
	keep_looping = 0;
}

int main(int argc, const char *argv[])
{
	if (argc < 2) {
		print_usage();
		exit(1);
	}
	parse_args(argc, argv);

	struct timespec start={0,0}, end={0,0};
	int nsec_per_frame = MILLISECONDS_PER_FRAME*1000; // *1000 to turn msec into nsec

	XDPY = XOpenDisplay(NULL);
    XSCRN_NUM = DefaultScreen(XDPY);
    XSCRN = ScreenOfDisplay(XDPY, XSCRN_NUM);
    ROOT_WIN = RootWindow(XDPY, XSCRN_NUM);
    COLORMAP = DefaultColormap(XDPY, XSCRN_NUM);
    VISUAL = DefaultVisual(XDPY, XSCRN_NUM);
    BITDEPTH = DefaultDepth(XDPY, XSCRN_NUM);

	imlib_context_set_display(XDPY);
	imlib_context_set_visual(VISUAL);
	imlib_context_set_colormap(COLORMAP);

	Imlib_Image *entries = load_images_from_folder_formatted(FOLDER_NAME, FRAME_FILE_NAME_FORMAT);
	if (!entries) {
		printf("Failed loading images from the folder %s\n", FOLDER_NAME);
		printf("Format: %s\n", FRAME_FILE_NAME_FORMAT);
		exit(1);
	}

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
	signal(SIGINT, exit_loop);
	signal(SIGTERM, exit_loop);
	keep_looping = 1;
	while (keep_looping) {
		current_pixmap = *(pixmaps+current);
		set_pixmap_property(current_pixmap);

		XSetWindowBackgroundPixmap(XDPY, ROOT_WIN, current_pixmap);
		XClearWindow(XDPY, ROOT_WIN);

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

	free(pixmaps);
	free(entries);

	return 0;
}
