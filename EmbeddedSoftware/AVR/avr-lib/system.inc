# System specific configuration file

# Define the command used to upload BIOS to the node
# It will be executed as $(UPLOAD) $(ULFLAGS).
# $(MCU) is the AVR to use and the file to upload is $(PRG).hex

UPLOAD         = avrdude
#avrispv2 avr910
ULFLAGS        = -p $(MCU) -P /dev/ttyS0 -c avr910 -b 115200 -U flash:w:$(PRG).hex

AVRDUDE_WRITE_FUSEBITS = -u
ifdef EXTFUSE
AVRDUDE_WRITE_FUSEBITS += -U efuse:w:$(EXTFUSE):m
endif
ifdef HIGHFUSE
AVRDUDE_WRITE_FUSEBITS += -U hfuse:w:$(HIGHFUSE):m
endif
ifdef LOWFUSE
AVRDUDE_WRITE_FUSEBITS += -U lfuse:w:$(LOWFUSE):m
endif

#comment this line for not programming fusebits
ULFLAGS      += $(AVRDUDE_WRITE_FUSEBITS)