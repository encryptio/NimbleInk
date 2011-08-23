from setuptools import setup
setup(
    app=["view.py"],
    setup_requires=["py2app"],
    options={
        'py2app': {
            'includes': ['OpenGL.GL', 'ctypes.util'],
        },
    },
)
