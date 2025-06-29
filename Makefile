#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = gcc

# Add include and lib paths manually
CFLAGS := -Wall -Wextra -g -IC:/msys64/ucrt64/include
LFLAGS := -LC:/msys64/ucrt64/lib -lmicrohttpd -lcurl -lcjson -lsqlite3 -lrpcrt4 -lcrypto -lssl

# define output directory
OUTPUT := output

# define source directory
SRC := src
ROUTES := routes routes/api routes/api/24scope lib

# define include directory
INCLUDE := include include/curl include/cjson

# define lib directory
LIB := lib

ifeq ($(OS),Windows_NT)
MAIN := main.exe
SOURCEDIRS := $(SRC) $(ROUTES)
INCLUDEDIRS := $(INCLUDE)
LIBDIRS := $(LIB)
FIXPATH = $(subst /,\,$1)
RM := cmd /c del /q /f
MD := mkdir
else
MAIN := main
SOURCEDIRS := $(shell find $(SRC) -type d)
INCLUDEDIRS := $(shell find $(INCLUDE) -type d)
LIBDIRS := $(shell find $(LIB) -type d)
FIXPATH = $1
RM = rm -f
MD := mkdir -p
endif

# additional include paths if needed
INCLUDES := $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

# additional lib paths if needed
LIBS := $(patsubst %,-L%, $(LIBDIRS:%/=%))

# define the C source files
SOURCES := $(wildcard $(patsubst %,%/*.c, $(SOURCEDIRS)))

# define the C object files 
OBJECTS := $(SOURCES:.c=.o)

# define the dependency output files
DEPS := $(OBJECTS:.o=.d)

# output binary with fixed path
OUTPUTMAIN := $(call FIXPATH,$(OUTPUT)/$(MAIN))

all: $(OUTPUT) $(MAIN)
	@echo Executing 'all' complete!

$(OUTPUT):
	$(MD) $(OUTPUT)

$(MAIN): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(OUTPUTMAIN) $(OBJECTS) $(LFLAGS) $(LIBS)

# include dependency files if they exist
-include $(DEPS)

# build object files with dependency generation
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -MMD $< -o $@

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(call FIXPATH,$(OBJECTS))
	$(RM) $(call FIXPATH,$(DEPS))
	@echo Cleanup complete!

run: all
	./$(OUTPUTMAIN)
	@echo Executing 'run: all' complete!
