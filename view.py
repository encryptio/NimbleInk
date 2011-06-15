#!/usr/bin/env python2.7

import sys
import time

from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

from imagezipper import ImageZipper
import imagelister

from animator import *

animationLength = 1.0/5
arrowBounceAnimationLength = 1.0/3
mouseCursorTimerFuzziness = 0.05
mouseCursorTimerLength    = 0.75

window_width  = 300
window_height = 300

last_animate_time = 0

rotation = 0
rotationParam = AnimatedParameter(0)
nowAnimImage = None
arrowBounceAnim = None
arrowBounceSide = 'left'

def initializeDisplay(w,h):
    glClearColor(0.0, 0.0, 0.0, 1.0)
    glClear(GL_COLOR_BUFFER_BIT)

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

    jumpToFitImage()

def animate():
    global last_animate_time
    this_animate_time = time.time()
    delta = this_animate_time - last_animate_time
    last_animate_time = this_animate_time

    changed = False
    changed |= rotationParam.step(delta)
    changed |= nowAnimImage.step(delta)
    if arrowBounceAnim:
        changed |= arrowBounceAnim.step(delta)

    if not changed:
        stopAnimation()

    glutPostRedisplay()

animationRunning = False
def startAnimation():
    global animationRunning
    if not animationRunning:
        animationRunning = True
        glutIdleFunc(animate)

        global last_animate_time
        last_animate_time = time.time()

def stopAnimation():
    global animationRunning
    animationRunning = False
    glutIdleFunc(None)

def display():
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    global window_width, window_height
    wid, hei = window_width, window_height

    glPushMatrix()
    glTranslatef( wid/2, hei/2,0)
    glRotatef(rotationParam.at,0,0,1)
    #glTranslatef(-wid/2,-hei/2,0)

    global nowAnimImage
    nowAnimImage.drawCentered()

    glPopMatrix()

    if arrowBounceAnim:
        if arrowBounceSide == 'left':
            arrowBounceAnim.draw(100,hei/2, 150,150)
        else:
            arrowBounceAnim.draw(wid-100,hei/2, 150,150)

    glutSwapBuffers()

def keyboard(key, x, y):
    print "keyboard(%d,%d,%d)" % (ord(key), x, y)
    global rotation
    if key == chr(27) or key == 'q' or key == 'Q':
        sys.exit(0)
    elif key == 'r':
        rotation += 1
        rotationParam.animateTo(rotation*90, animationLength)
        animToFitImage(animationLength)
        startAnimation()
    elif key == 'R':
        rotation -= 1
        rotationParam.animateTo(rotation*90, animationLength)
        animToFitImage(animationLength)
        startAnimation()
    elif key == 'f' or key == 'F':
        glutFullScreen()

        # TODO: wtf game mode
        #if glutGameModeGet(GLUT_GAME_MODE_ACTIVE):
        #    glutLeaveGameMode()
        #else:
        #    glutEnterGameMode()
        #setAllCallbacks()

def jumpToFitImage():
    wid,hei = window_width, window_height
    if rotation % 2 == 1:
        wid,hei = hei,wid
    nowAnimImage.jumpToFit(0,0,wid,hei)

def animToFitImage(time):
    wid,hei = window_width, window_height
    if rotation % 2 == 1:
        wid,hei = hei,wid
    nowAnimImage.animateToFit(0,0,wid,hei,time)
    startAnimation()

def specialKeyboard(key, x, y):
    print "specialKeyboard(%d,%d,%d)" % (key, x, y)
    global zipper, nowAnimImage, arrowBounceAnim, arrowBounceSide
    if key == GLUT_KEY_RIGHT:
        if zipper.stepForward():
            nowAnimImage = AnimatedImage(zipper.nowImage)
            jumpToFitImage()
            glutPostRedisplay()
        else:
            arrowBounceAnim = AnimatedArrowBounce(arrowBounceAnimationLength, 'right')
            arrowBounceSide = 'right'
            startAnimation()
            glutPostRedisplay()
    elif key == GLUT_KEY_LEFT:
        if zipper.stepBackward():
            nowAnimImage = AnimatedImage(zipper.nowImage)
            jumpToFitImage()
            glutPostRedisplay()
        else:
            arrowBounceAnim = AnimatedArrowBounce(arrowBounceAnimationLength, 'left')
            arrowBounceSide = 'left'
            startAnimation()
            glutPostRedisplay()

currentMoveTimer = 0
lastTimerSetTime = 0
def mouseMove(x,y):
    print "mouseMove(%d,%d)" % (x, y)
    glutSetCursor(GLUT_CURSOR_INHERIT)
    
    global currentMoveTimer, lastTimerSetTime
    if time.time() - lastTimerSetTime > mouseCursorTimerFuzziness:
        currentMoveTimer += 1
        lastTimerSetTime = time.time()
        glutTimerFunc(int(mouseCursorTimerLength*1000), mouseTimeout, currentMoveTimer)

def mouseTimeout(value):
    print "mouseTimeout(%d)" % value
    if value == currentMoveTimer:
        glutSetCursor(GLUT_CURSOR_NONE)

def reshape(w,h):
    print "reshape(%d,%d)" % (w,h)
    initializeDisplay(w,h)

def setAllCallbacks():
    glutDisplayFunc(display)
    glutKeyboardFunc(keyboard)
    glutSpecialFunc(specialKeyboard)
    glutReshapeFunc(reshape)
    glutMotionFunc(mouseMove)
    glutPassiveMotionFunc(mouseMove)

    startAnimation()

def main():
    glutInit(sys.argv)
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE)
    
    windowContext = glutCreateWindow('comicviewer')
    glutSetWindow(windowContext)

    setAllCallbacks()

    print "imagelister"
    files, delete_items = imagelister.findAndExtractFiles(sys.argv[1:])

    print "load zipper"
    global zipper, nowAnimImage
    zipper = ImageZipper(files, delete_items)
    nowAnimImage = AnimatedImage(zipper.nowImage)
    jumpToFitImage()

    mouseMove(0,0)

    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

    print "mainloop"
    glutMainLoop()

if __name__ == '__main__':
    main()

