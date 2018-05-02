"""
Wrapper for PyMOL scripts to get PySide or PyQt

Useful link for PySide/PyQt4 differences:
https://deptinfo-ensip.univ-poitiers.fr/ENS/pyside-docs/pysideapi2.html

PyQt5/PyQt4 differences:
http://pyqt.sourceforge.net/Docs/PyQt5/pyqt4_differences.html
"""

from __future__ import absolute_import as _

DEBUG = False

PYQT_NAME = None

try:
    from pymol._Qt_pre import *
except ImportError:
    if DEBUG:
        print('import _Qt_pre failed')

if not PYQT_NAME:
    try:
        from PyQt5 import QtGui, QtCore, QtOpenGL
        PYQT_NAME = 'PyQt5'
    except ImportError:
        if DEBUG:
            print('import PyQt5 failed')

if not PYQT_NAME:
    try:
        __import__("sip").setapi("QString", 2)
        from PyQt4 import QtGui, QtCore, QtOpenGL
        PYQT_NAME = 'PyQt4'
    except ImportError:
        if DEBUG:
            print('import PyQt4 failed')

if not PYQT_NAME:
    try:
        from PySide import QtGui, QtCore, QtOpenGL
        PYQT_NAME = 'PySide'
    except ImportError:
        if DEBUG:
            print('import PySide failed')

if not PYQT_NAME:
    raise ImportError(__name__)

import os

# qtpy compatibility
os.environ['QT_API'] = PYQT_NAME

if PYQT_NAME == 'PyQt5':
    from PyQt5 import QtWidgets
    QtCoreModels = QtCore
else:
    QtWidgets = QtGui
    QtCoreModels = QtGui

if PYQT_NAME == 'PyQt4':
    QFileDialog = QtWidgets.QFileDialog
    QFileDialog.getOpenFileName = QFileDialog.getOpenFileNameAndFilter
    QFileDialog.getOpenFileNames = QFileDialog.getOpenFileNamesAndFilter
    QFileDialog.getSaveFileName = QFileDialog.getSaveFileNameAndFilter
    del QFileDialog

if PYQT_NAME[:4] == 'PyQt':
    QtCore.Signal = QtCore.pyqtSignal
    QtCore.Slot = QtCore.pyqtSlot
else:
    QtCore.QT_VERSION_STR = QtCore.__version__
    QtCore.QT_VERSION = (
            0x10000 * QtCore.__version_info__[0] +
            0x00100 * QtCore.__version_info__[1] +
            0x00001 * QtCore.__version_info__[2])

del os
