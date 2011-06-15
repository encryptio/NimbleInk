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

