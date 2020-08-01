
# asetroot
A lightweight animated wallpaper program, or if you will, **animated** setroot 

![asetroot demonstration](demo.gif)

## Building  
  
> pacman -S imlib2  
> make  

  
## Basic Usage  
  
asetroot \[FOLDER] ... \[-t milliseconds | -f format]  
Where \[FOLDER] is a folder with all the frames.  
Frames should have a file name that can be incremented using a printf format, standard is %05d.gif  
This program does not do any resizing or converting, if you wish to do that check out ImageMagick  
  
## Examples  

> \# to resize and split a gif into a folder   
> convert example.gif -coalesce -resize 1920x1080 examplefolder/%05d.gif   
> 
> \# activates the program with 50 milliseconds between each frame  
> asetroot examplefolder/ -t 50
