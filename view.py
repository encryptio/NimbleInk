#!/usr/bin/env python2

import sys

from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

from imagezipper import ImageZipper
import imagelister

import pygame

def initializeDisplay(w,h):
    glClearColor(0.0, 0.0, 0.0, 1.0)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    # world mapping is pixel mapping
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluOrtho2D(0,w,0,h)
    glMatrixMode(GL_MODELVIEW)

    # draw onto entire window
    glViewport(0,0,w,h)

    # enable texturing
    glEnable(GL_TEXTURE_2D)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

    global window_width, window_height
    window_width = w
    window_height = h

def display():
    print "display()"

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    global zipper, window_width, window_height
    zipper.nowImage.drawInside(0,0,window_width,window_height)

    print "  -> swap"
    glutSwapBuffers()

def keyboard(key, x, y):
    print "keyboard(%d,%d,%d)" % (ord(key), x, y)
    if key == chr(27) or key == 'q':
        sys.exit(0)

def specialKeyboard(key, x, y):
    print "specialKeyboard(%d,%d,%d)" % (key, x, y)
    global zipper
    if key == GLUT_KEY_RIGHT:
        if zipper.stepForward():
            glutPostRedisplay()
    elif key == GLUT_KEY_LEFT:
        if zipper.stepBackward():
            glutPostRedisplay()

def reshape(w,h):
    print "reshape(%d,%d)" % (w,h)
    initializeDisplay(w,h)

def main():
    glutInit(sys.argv)
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE)
    
    windowContext = glutCreateWindow('comicviewer')
    glutSetWindow(windowContext)

    glutDisplayFunc(display)
    glutKeyboardFunc(keyboard)
    glutSpecialFunc(specialKeyboard)
    glutReshapeFunc(reshape)

    print "imagelister"
    files, delete_items = imagelister.findAndExtractFiles(sys.argv[1:])

    print "load zipper"
    global zipper
    zipper = ImageZipper(files)

    print "mainloop"
    glutMainLoop()

if __name__ == '__main__':
    main()

