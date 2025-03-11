# Project Name
TARGET = MatrixReverb

# Sources
CPP_SOURCES += MatrixReverb.cpp \
							 Reverb.cpp

C_SOURCES += CMSIS/arm_cos_f32.c \
						 CMSIS/arm_sin_f32.c \
						 CMSIS/arm_fill_f32.c \
						 CMSIS/arm_copy_f32.c \
						 CMSIS/arm_mat_init_f32.c \
						 CMSIS/arm_mat_mult_f32.c \
						 CMSIS/arm_common_tables.c
# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
