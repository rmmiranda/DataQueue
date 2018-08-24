#
# Cross-compilation flags
#
PREFIX := arm-none-eabi
CC  := $(PREFIX)-gcc
AR  := $(PREFIX)-ar

#
# Compiler flags
#
FLAGS := -O3 -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard

#
# Include directories
#
INC := -Iinc \
	-I../segger-emfile/FS \
	-I../segger-emfile/SEGGER \
	-I../segger-emfile/Config \

#
# Libraries
#
LIBS := -L../segger-emfile/build \
	-lemfile

#
# Output directory where .o files and static library will be generated
#
OBJ_DIR = build

#
# Source files
#
SRC := src/dataqueue.c

#
# Data queue configuration flags
#
DATA_QUEUE_PSL := -DPSL_LINUX
DATA_QUEUE_FSAL := -DFSAL_SEGGER_EMFILE

#
# Conditional inclusion of source files based on PSL selection
#
ifeq ($(DATA_QUEUE_PSL),-DPSL_LINUX)
		SRC += psl/linux/psl.c
		SRC += psl/linux/test.c
		INC += -Ipsl
else
		$(error No PSL (Platform Software Layer) defined in Makefile)
endif

#
# Conditional inclusion of source files based on FSAL selection
#
ifeq ($(DATA_QUEUE_FSAL),-DFSAL_LINUX_EXT4)
		SRC += fsal/linux_ext4/fsal.c
		INC += fsal/linux_ext4
else ifeq ($(DATA_QUEUE_FSAL),-DFSAL_SEGGER_EMFILE)
		SRC += fsal/segger-emfile/fsal.c
		INC += -Ifsal/segger-emfile
else ifeq ($(DATA_QUEUE_FSAL),-DFSAL_SEGGER_STUB)
		SRC += fsal/stub/fsal.c
		INC += fsal/stub
else
		$(error No FSAL (File System Abstraction Layer) defined in Makefile)
endif

#
# Converting source files into objects using path substitution
#
OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SRC)))

#
# Target name and path
#
TARGET = $(OBJ_DIR)/libdataqueue.a

$(TARGET): build/dataqueue.o build/psl.o build/fsal.o
	$(AR) rcs $@ $^

#
# Rule to build object files from source files in multiple directories
#
$(OBJ): $(SRC)
		$(CC) $(FLAGS) $(INC) $(LIBS) $(DATA_QUEUE_PSL) $(DATA_QUEUE_FSAL) -c $< -o $@

#
# Rule to clean all compilation artifacts
#
clean:
	rm -rf $(OBJ_DIR)

#
# Create artifact directory on each run if it doesn't already exist
#
$(shell mkdir -p $(OBJ_DIR))

print-%  : ; @echo $* = $($*)
