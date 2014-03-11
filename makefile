# ---------------------------------------------------------
PROJECT = test.exe
MAIN_SRC_FILE = examples
MAIN_SRC_PATH = .
OBJFILES = $(MAIN_SRC_PATH)\$(MAIN_SRC_FILE).obj $(MAIN_SRC_PATH)\stdafx.obj $(MAIN_SRC_PATH)\txt2img.obj
DEBUG = /Od /Oy- /D _DEBUG
# ---------------------------------------------------------
USER_DEFINES = /D WIN32
USER_INCLUDE = 
SYSTEM_INCLUDE = -I "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include"
SYSTEM_LIB_PATH = "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib" 
LIBS_PATH = 

CC = cl.exe
CFLAGS = /nologo $(DEBUG) /Zi /W3 /WX- /Gm /EHsc /RTC1 /GS /fp:precise /Zc:forScope /Gd /MTd -c

SYSTEM_LIBS = "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" 
LD = link.exe
LDFLAGS = /NOLOGO /DEBUG /INCREMENTAL /ALLOWISOLATION /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:x86 /OUT:$(PROJECT) /LIBPATH:$(SYSTEM_LIB_PATH)
# ---------------------------------------------------------
$(PROJECT): $(OBJFILES)
	$(CC) /MTd $(OBJFILES) $(SYSTEM_LIBS) /link $(LDFLAGS) 


clean:
	-del $(OBJFILES)
	-del $(PROJECT)


.PHONY: $(PROJECT)
# ---------------------------------------------------------
.cpp.obj:
    $(CC) $(SYSTEM_INCLUDE) $(USER_DEFINES) $(USER_INCLUDE) $(CFLAGS) $<
# ---------------------------------------------------------
# ---------------------------------------------------------
