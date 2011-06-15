from drawimage import DrawImage

import threading
import shutil

class ImageZipper:
    def __init__(self, paths, deleteOnDel=[]):
        self.backward = []
        self.now = paths[0]
        self.forward = paths[1:]

        self.deleteOnDel = deleteOnDel
        
        self.backwardImage = None
        self.nowImage = DrawImage(self.now)
        self.forwardImage = None

        self.loadLock = threading.Lock()
        self.preload()

    def stepForward(self):
        if len(self.forward):
            with self.loadLock:
                self.backward.insert(0, self.now)
                self.now = self.forward.pop(0)

                self.backwardImage = self.nowImage
                self.nowImage = self.forwardImage
                self.forwardImage = None

                self.nowImage.initializeForGL()

            self.preload()

            return True
        else:
            return False

    def stepBackward(self):
        if len(self.backward):
            with self.loadLock:
                self.forward.insert(0, self.now)
                self.now = self.backward.pop(0)

                self.forwardImage = self.nowImage
                self.nowImage = self.backwardImage
                self.backwardImage = None

                self.nowImage.initializeForGL()

            self.preload()

            return True
        else:
            return False

    def preload(self):
        threadStarted = threading.Semaphore(0)

        def preloadThread(self, semaphore):
            with self.loadLock:
                semaphore.release()
                if self.forwardImage is None and len(self.forward):
                    self.forwardImage = DrawImage(self.forward[0], False)
                if self.backwardImage is None and len(self.backward):
                    self.backwardImage = DrawImage(self.backward[0], False)

        threading.Thread(target=preloadThread, args=(self, threadStarted)).start()

        # wait for the thread to obtain the loadLock
        threadStarted.acquire()

    def __del__(self):
        for f in self.deleteOnDel:
            print "removing temporary directory %s" % f
            shutil.rmtree(f)

