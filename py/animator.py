from OpenGL.GL import *

import math

class AnimatedParameter:
    def __init__(self, val):
        self.jump(val)

    def step(self, size):
        if self.pos >= 1:
            self.jump(self.to)
            return False

        self.pos += size/self.time

        if self.pos >= 1:
            self.jump(self.to)
            return False

        part = (self.pos + 1 - math.cos(self.pos*math.pi))/3
        self.at = self.to*part + self.fro*(1-part)

        return True

    def jump(self, new):
        self.fro = self.to = self.at = float(new)
        self.pos = 0
        self.time = 0.01

    def animateTo(self, to, time):
        self.fro = self.at
        self.to = to
        self.pos = 0
        self.time = time

class AnimatedImage:
    def __init__(self, drawimage):
        self.x = AnimatedParameter(0)
        self.y = AnimatedParameter(0)
        self.w = AnimatedParameter(drawimage.width)
        self.h = AnimatedParameter(drawimage.height)

        self.image = drawimage

    def jumpTo(self,x,y,w,h):
        self.x.jump(x)
        self.y.jump(y)
        self.w.jump(w)
        self.h.jump(h)

    def jumpToFit(self,x,y,w,h):
        nw,nh = self.image.fitSize(w,h)
        self.jumpTo(x, y, nw, nh)

    def animateTo(self,x,y,w,h,time):
        self.x.animateTo(x,time)
        self.y.animateTo(y,time)
        self.w.animateTo(w,time)
        self.h.animateTo(h,time)

    def animateToFit(self,x,y,w,h,time):
        nw,nh = self.image.fitSize(w,h)
        self.animateTo(x, y, nw, nh, time)

    def draw(self):
        self.image.draw(self.x.at, self.y.at, self.w.at, self.h.at)

    def drawCentered(self):
        self.image.draw(self.x.at - self.w.at/2, self.y.at - self.h.at/2, self.w.at, self.h.at)

    def step(self, time):
        changed = False
        changed |= self.x.step(time)
        changed |= self.y.step(time)
        changed |= self.w.step(time)
        changed |= self.h.step(time)
        return changed

class AnimatedArrowBounce:
    # Timeline for animation:
    # 0 ........... 0.5 ................. 2
    #  ^Move to bar^   ^Bounce off of bar^
    #                  ^     Fade Out    ^

    def __init__(self, time, side):
        self.now = 0
        self.time = time
        self.side = side

        self.barDL = None
        self.arrowDL = None
        self.glInitialized = False

    def step(self, time):
        if self.now >= 2:
            return False

        self.now += time/self.time*2

        if self.now >= 2:
            self.now = 2
            return False

        return True

    def calcArrowOffset(self):
        if self.now < 0.5:
            return 0.5-self.now
        else:
            return 1-math.exp(-(self.now-0.5))

    def calcAlpha(self):
        if self.now < 0.5:
            return 1.0
        else:
            return 1.0-(self.now-0.5)*2/3.0

    def draw(self, x, y, w, h):
        if not self.glInitialized:
            self.createDisplayLists()
            self.glInitialized = True

        glPushMatrix()

        glTranslatef(x,y,0)

        ### draw darkening background
        glColor4f(0,0,0,self.calcAlpha()*0.4)

        glPushMatrix()
        glScalef(w*1.2,h*1.2,1)
        if self.side == 'left':
            glScalef(-1,1,1)
        glTranslatef(-.25,0,0)
        glScalef(1.5,1,1)
        glTranslatef(-.5,-.5,0)

        glCallList(self.barDL)
        glPopMatrix()
        
        ### draw arrow
        glColor4f(1,1,1,self.calcAlpha())

        glPushMatrix()
        glScalef(w,h,1)
        if self.side == 'left':
            glScalef(-1,1,1)
        glTranslatef(-self.calcArrowOffset(), 0, 0)
        glScalef(0.8,0.8,0.8)
        glTranslatef(-.5,-.5,0)

        glCallList(self.arrowDL)
        glPopMatrix()

        ### draw bar
        if self.side == 'left':
            glScalef(-1,1,1)
        glScalef(w,h,1)
        glTranslatef(.4,-.5,0)
        glScalef(0.1,1,1)

        glCallList(self.barDL)

        glPopMatrix()

    def createArrowDisplayList(self):
        # 1       +
        #         |\
        # .7  +---+ \
        # .3  +---+ /
        #         |/
        # 0       +
        #     0   .6 1

        self.arrowDL = glGenLists(1)

        glNewList(self.arrowDL, GL_COMPILE)
        glBegin(GL_TRIANGLE_FAN)
        glVertex2f(0.8,0.5)

        glVertex2f(.6,.7)
        glVertex2f(.6,1)
        glVertex2f(1,0.5)
        glVertex2f(.6,0)
        glVertex2f(.6,.3)
        glVertex2f(0,.3)
        glVertex2f(0,.7)
        glVertex2f(.6,.7)

        glEnd()
        glEndList()

    def createBarDisplayList(self):
        self.barDL = glGenLists(1)

        glNewList(self.barDL, GL_COMPILE)
        glBegin(GL_QUADS)

        glVertex2f(0,0)
        glVertex2f(0,1)
        glVertex2f(1,1)
        glVertex2f(1,0)

        glEnd()
        glEndList()

    def createDisplayLists(self):
        self.createArrowDisplayList()
        self.createBarDisplayList()

    def __del__(self):
        if self.barDL:
            glDeleteLists(self.barDL, 1)
        if self.arrowDL:
            glDeleteLists(self.arrowDL, 1)

