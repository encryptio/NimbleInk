from OpenGL.GL import *

import Image
import time

class DrawImage:
    def __init__(self, path, initializeGL=True):
        startTime = time.time()

        self.imagePath = path

        # TODO: about 4x slower than pygame image loading
        img = Image.open(path)
        self.width = img.size[0]
        self.height = img.size[1]
        try:
            self.textureData = img.tostring("raw", "RGBA", 0, -1)
        except SystemError:
            try:
                self.textureData = img.tostring("raw", "RGBX", 0, -1)
            except SystemError:
                self.textureData = img.convert("RGBA").transpose(Image.FLIP_TOP_BOTTOM).tostring()

        self.glInitialized = False

        self.texture = None
        self.displayList = None

        endTime = time.time()
        print "DrawImage created in %.2f seconds: %s" % (endTime-startTime, path)

        if initializeGL:
            self.initializeForGL()

    def initializeForGL(self):
        if not self.glInitialized:
            print "initializing gl for " + self.imagePath
            self.glInitialized = True
            self.createTexture()
            self.createDisplayList()

    def createDisplayList(self):
        self.displayList = glGenLists(1)

        glNewList(self.displayList, GL_COMPILE)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
        glBindTexture(GL_TEXTURE_2D, self.texture)
        glBegin(GL_QUADS)
        glTexCoord2f(0, 0); glVertex2f(0, 0) # bottom left
        glTexCoord2f(0, 1); glVertex2f(0, 1) # top left
        glTexCoord2f(1, 1); glVertex2f(1, 1) # top right
        glTexCoord2f(1, 0); glVertex2f(1, 0) # bottom right
        glEnd()
        glBindTexture(GL_TEXTURE_2D, 0)
        glEndList()

    def destroyDisplayList(self):
        if not self.displayList is None:
            glDeleteLists(self.displayList, 1)

    def createTexture(self):
        self.texture = glGenTextures(1)
        glBindTexture(GL_TEXTURE_2D, self.texture)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self.width, self.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, self.textureData)

        self.textureData = None

    def destroyTexture(self):
        if not self.texture is None:
            glDeleteTextures(self.texture)

    def draw(self, x0=0.0, y0=0.0, w=None, h=None):
        if w is None or h is None:
            w = self.width
            h = self.height
        glPushMatrix()
        glTranslatef(x0,y0,0)
        glScalef(w,h,1)
        glColor4f(1,1,1,1)
        glCallList(self.displayList)
        glPopMatrix()

    def fitSize(self, w, h):
        mw = self.width
        mh = self.height

        sf = float(w)/mw
        if sf*mh > h:
            sf = float(h)/mh

        return mw*sf, mh*sf

    def drawInside(self, x0, y0, w, h):
        ew, eh = self.fitSize(w,h)

        self.draw(x0 + (w-ew)/2, y0 + (h-eh)/2, ew, eh)

    def __del__(self):
        print "DrawImage destroyed: " + self.imagePath
        self.destroyTexture()
        self.destroyDisplayList()

