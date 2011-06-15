import tempfile
import os
import os.path
import random
import subprocess
import shutil
import time

archive_extensions = "7z cbz cbr zip rar ".split(" ")
image_extensions   = "jpg jpeg gif png bmp".split(" ")

def findAndExtractFiles(file_list):
    files = []
    delete_dirs = []

    for f in file_list:
        if os.path.isdir(f):
            listing = os.listdir(f)
            listing.sort()
            for sf in listing:
                subfiles, subdels = findAndExtractFiles( [os.path.join(f,sf)] )

                files.extend(subfiles)
                delete_dirs.extend(subdels)

        elif os.path.isfile(f):
            root, ext = os.path.splitext(f)
            if ext[1:].lower() in image_extensions:
                files.append(f)
            elif ext[1:].lower() in archive_extensions:
                subfiles, subdels = openArchive( f )

                files.extend(subfiles)
                delete_dirs.extend(subdels)
            else:
                print "file ignored: " + f

        else:
            print "o shit it aint a file or a directory fuuuuu (" + f + ")"

    return files, delete_dirs

def openArchive(path):
    tempPath = None
    while tempPath is None or os.path.lexists(tempPath):
        tempPath = os.path.join(tempfile.gettempdir(), "comicviewer-%d" % random.randint(0,10000000))

    os.mkdir(tempPath)

    ret = subprocess.call(["7z","x","-bd","-o"+tempPath,path])
    if ret != 0:
        shutil.rmtree(tempPath)
        print "Couldn't extract " + path + ": return code %d" % ret

    files, delete_dirs = findAndExtractFiles([tempPath])

    delete_dirs.append(tempPath)

    return files, delete_dirs

