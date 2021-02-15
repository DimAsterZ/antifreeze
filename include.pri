defineReplace(copyFile){
	SRC_FILE = $$shell_path($$1)
	DST_PATH = $$shell_path($$2)
	message($$quote(cmd /c copy /y $$escape_expand(\")$${SRC_FILE}$$escape_expand(\") $$escape_expand(\")$${DST_PATH}$$escape_expand(\")$$escape_expand(\n\t)))
	return ($$quote(cmd /c copy /y $$escape_expand(\")$${SRC_FILE}$$escape_expand(\") $$escape_expand(\")$${DST_PATH}$$escape_expand(\")$$escape_expand(\n\t)))
}

defineReplace(makeDir){
	return ($$quote(cmd /c if exist $$escape_expand(\")$$1$$escape_expand(\") (@echo dir already exists) else (mkdir $$escape_expand(\")$$1$$escape_expand(\")) $$2$$escape_expand(\n\t)))
}

defineReplace(copyFile_unix_noerr){
        SRC_PATH = $$shell_path($$1)
        SRC_FILES = $$shell_path($$2)
        DST_PATH = $$shell_path($$3)
        return ($$quote(\cp -v $$escape_expand(\")$${SRC_PATH}$$escape_expand(\")$$SRC_FILES $$escape_expand(\")$${DST_PATH}$$escape_expand(\") || : $$escape_expand(\n\t)))
}

############### include multithreading #######################
include($$PWD/../antifreeze/multithreading/include.pri)
##############################################################

################### include antifreeze #######################
INCLUDEPATH += $$PWD/../antifreeze/include
!contains(DEFINES, ANTIFREEZE_LIBRARY) {
    win32 {
            CONFIG(release, debug|release) {
                    LIBS += -L$$PWD/../antifreeze/bin/ -lantifreeze
                    QMAKE_POST_LINK += $$copyFile($$PWD/../antifreeze/bin/antifreeze.dll, $$DESTDIR/*.dll)
            }
            else:CONFIG(debug, debug|release) {
                    LIBS += -L$$PWD/../antifreeze/bin/ -lantifreezed
                    QMAKE_POST_LINK += $$copyFile($$PWD/../antifreeze/bin/antifreezed.dll, $$DESTDIR/*.dll)
            }
    } else:unix {
        CONFIG(debug, debug|release) {
            LIBS += -lantifreezed
        }
        else:CONFIG(release, debug|release) {
            LIBS += -lantifreeze
        }
    }
}
##############################################################
