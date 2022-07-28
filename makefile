TARGET=helix

include ../makefile

FILES = $(TARGET) $(LIB)/sprintf $(LIB)/string $(LIB)/stdio $(LIB)/graphics $(LIB)/floatimpl $(LIB)/math

ASM_LIST := $(foreach file,$(FILES), $(file).s) $(LIB)/keyboard.s $(LIB)/files.s $(LIB)/graphics320.s $(LIB)/graphics640.s $(LIB)/fonts.s $(LIB)/consts.s

sd_copy:
	cp $(TARGET).bin ../helix.bin

run: compile assemble
	java -jar ../../FPGAEmulator32/FPGAEmulator32.jar