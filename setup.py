from distutils.core import setup, Extension

setup(
	name = "iphone",
	version = "1.0",
	ext_modules = [Extension(
		"iphone", 
		["main.c", "../libirecovery/src/libirecovery.c"],
		include_dirs=['/opt/local/include', '../libirecovery/include'],
		library_dirs=['/opt/local/lib'],
		libraries=['usb-1.0']
	)]
)
