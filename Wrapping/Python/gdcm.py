############################################################################
#
#  Program: GDCM (Grassroots DICOM). A DICOM library
#  Module:  $URL$
#
#  Copyright (c) 2006-2008 Mathieu Malaterre
#  All rights reserved.
#  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
############################################################################

""" This module loads all the classes from the GDCM library into
its namespace.  This is a required module."""

import os
import sys

# GDCM_WHEREAMI is a secret variable used to passed the location of the gdcm.py
# to the global singleton initialization internal to gdcm:
# I cannot use neither getcwd nor /proc/self/exe since the process really is: `python`
# Solution:
# 1. Create a dummy env var within python
# 2. Load the gdcm_wrapped interface
# 3. The internal gdcm C++ will inspect for this variable, if set: use it !
#if os.environ["GDCM_WHEREAMI"]:
#  os.exit(1)
# WARNING: this is currently the implementation chosen to pass in the information
# during the singleton initalization, but is subject to change without notice
# do not expect this env var to be present at any time in your project

def main_is_frozen():
  return hasattr(sys, "frozen")

if os.name == 'posix':
  # extremely important !
  # http://gcc.gnu.org/faq.html#dso
  # http://mail.python.org/pipermail/python-dev/2002-May/023923.html
  # http://wiki.python.org/moin/boost.python/CrossExtensionModuleDependencies
  orig_dlopen_flags = sys.getdlopenflags()
  try:
    import dl
  except ImportError:
    # are we on AMD64 ?
    try:
      import DLFCN as dl
    except ImportError:
      print "Could not import dl"
      dl = None
  if dl:
    #print "dl was imported"
    #sys.setdlopenflags(dl.RTLD_LAZY|dl.RTLD_GLOBAL)
    sys.setdlopenflags(dl.RTLD_NOW|dl.RTLD_GLOBAL)
  from gdcmswig import *
  # revert:
  sys.setdlopenflags(orig_dlopen_flags)
  del dl
  del orig_dlopen_flags
else:
  from gdcmswig import *

# To finish up with module loading let's do some more stuff, like path to resource init:
if main_is_frozen():
  Global.GetInstance().Prepend( os.path.dirname(sys.executable) )
else:
  Global.GetInstance().Prepend( os.path.dirname(__file__) + "/../../../"  + GDCM_INSTALL_DATA_DIR + "/XML/" )

# Do it afterward so that it comes in first in the list
if os.environ["GDCM_RESOURCES_PATH"]:
  Global.GetInstance().Prepend( os.environ["GDCM_RESOURCES_PATH"] )

# bye bye
# once the process dies, the changed environment dies with it.
del os,sys

