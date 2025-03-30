CC=sdcc
IHX2SMS=ihx2sms
DEVKITSMS_BASE=E:/devkitSMS-master
SMSLIB_BASE=$(DEVKITSMS_BASE)/SGlib
SMSLIB_INCDIR=$(SMSLIB_BASE)/src
CRT0=$(DEVKITSMS_BASE)/crt0/crt0_sg.rel
#SGLIB_LIB=$(SMSLIB_BASE)/SMSlib.lib          # Use distributed lib
SGLIB_LIB=$(SMSLIB_BASE)/src/SGlib.rel      # Use locally compiled lib

CFLAGS=-mz80 -I$(SMSLIB_INCDIR)
LDFLAGS=-mz80 --no-std-crt0 --data-loc 0xC000

PROGNAME=menu

OBJS=$(PROGNAME).rel

all: $(PROGNAME).sg

%.rel: %.c
	$(CC) $(CFLAGS) -c $<

%.rel: %.c %.h
	$(CC) $(CFLAGS) -c $<

$(PROGNAME).ihx: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(CRT0) $(SGLIB_LIB) $^

$(PROGNAME).sg: $(PROGNAME).ihx
	$(IHX2SMS) $< $@

clean:
	del -f *.rel *.ihx *.asm *.sym *.lst *.noi *.lk *.map *.sms *.gg

# When a linking error occurs, sdcc returns an error and make fails. However
# a .ihx output file still gets created. This leads to make thinking that the .ihx
# file is up to date the next time it runs... This forces linking to take place
# every time make is called.
.PHONY: $(PROGNAME).ihx
