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
INC := -Iinc

#
# Output directory where .o files and static library will be generated
#
OBJ_DIR = build

#
# Data queue configuration flags
#
DATA_QUEUE_PSL := -DPSL_LINUX
DATA_QUEUE_FSAL := -DFSAL_SEGGER_EMFILE

INC += -Ipsl
INC += -Ifsal/segger-emfile
INC += -I../segger-emfile/FS
INC += -I../segger-emfile/SEGGER
INC += -I../segger-emfile/Config
INC += -L../segger-emfile/build -lemfile

#
# Target name and path
#
TARGET = $(OBJ_DIR)/libdataqueue.a

$(TARGET): build/dataqueue.o build/psl.o build/fsal.o build/test.o
	$(AR) rcs $@ $^

build/dataqueue.o: src/dataqueue.c
		$(CC) $(FLAGS) $(INC) $(DATA_QUEUE_PSL) $(DATA_QUEUE_FSAL) -c $< -o $@

build/fsal.o: fsal/segger-emfile/fsal.c
		$(CC) $(FLAGS) $(INC) $(DATA_QUEUE_PSL) $(DATA_QUEUE_FSAL) -c $< -o $@

build/psl.o: psl/linux/psl.c
		$(CC) $(FLAGS) $(INC) $(DATA_QUEUE_PSL) $(DATA_QUEUE_FSAL) -c $< -o $@

build/test.o: psl/linux/test.c
		$(CC) $(FLAGS) $(INC) $(DATA_QUEUE_PSL) $(DATA_QUEUE_FSAL) -c $< -o $@

#
# Rule to clean all compilation artifacts
#
clean:
	rm -rf $(OBJ_DIR)

#
# Create artifact directory on each run if it doesn't already exist
#
$(shell mkdir -p $(OBJ_DIR))
