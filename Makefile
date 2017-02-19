# Make File for Apple IIe Emulator for PalmOS 3.1
VERSION_MAJOR =0
VERSION_MINOR =7.3
CFLAGS = -Os
# -g -gstabs
# -DDEBUG
AFLAGS = -m68000 -pic -disp-size-default-16
# -D --gstabs 

INCLUDES =
APPALM_OBJS = obj/appalm.o obj/6502.o obj/memio.o obj/video.o obj/vidclr.o obj/fonts7x8.o obj/fonts4x6.o
APPALM_BINS = obj/MBAR0bb8.bin obj/tAIB03e8.bin obj/tAIB03e9.bin obj/Talt08fc.bin obj/Talt09c4.bin obj/Talt0a28.bin obj/tFRM03e8.bin obj/tFRM044c.bin obj/tFRM04b0.bin obj/tver03e8.bin
APPALM_M68KEXECUTABLE = obj/appalm
APPALM = appalm.prc
A2MGR_OBJS = obj/a2mgr.o
A2MGR_BINS = obj/tAIB03e8.bin obj/tAIB03e9.bin obj/Talt238c.bin obj/Talt23f0.bin obj/tFRM2328.bin obj/tver03e8.bin 
A2MGR_M68KEXECUTABLE = obj/a2mgr
A2MGR = a2mgr.prc
DSK2PDB = utils/dsk2pdb

BUILDPRC = build-prc
PILRC    = pilrc -I src
GCC      = m68k-palmos-gcc
AS       = m68k-palmos-as
TAR      = tar
ZIP	 = zip

all:	$(APPALM) $(A2MGR) $(DSK2PDB)

clean: cleansrc cleanobj

cleansrc:
	rm -f src/*.c~
	rm -f src/*.h~
	rm -f src/*.bak
	rm -f src/tags
	rm -f utils/dsk2pdb
	rm -f tags

cleanobj:
	rm -f obj/*.o
	rm -f obj/*.s
	rm -f obj/*.bin
	rm -f obj/*.stamp
	rm -f $(APPALM_M68KEXECUTABLE)
	rm -f $(APPALM)
	rm -f $(A2MGR_M68KEXECUTABLE)
	rm -f $(A2MGR)

dist:
	mkdir appalm-$(VERSION_MAJOR).$(VERSION_MINOR)
	mkdir appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src
	mkdir appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src/fonts4x6
	mkdir appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src/fonts7x8
	mkdir appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/obj
	mkdir appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/utils
	cp src/*.c appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src
	cp src/*.h appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src
	cp src/*.asm appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src
	cp src/*.rcp appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src
	cp src/fonts4x6/*.bmp appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src/fonts4x6
	cp src/fonts7x8/*.bmp appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/src/fonts7x8
	cp utils/*.c appalm-$(VERSION_MAJOR).$(VERSION_MINOR)/utils
	cp Makefile appalm-$(VERSION_MAJOR).$(VERSION_MINOR)
	cp README appalm-$(VERSION_MAJOR).$(VERSION_MINOR)
	$(TAR) czf appalm-$(VERSION_MAJOR).$(VERSION_MINOR).tar.gz appalm-$(VERSION_MAJOR).$(VERSION_MINOR)
	$(ZIP) appalm-$(VERSION_MAJOR).$(VERSION_MINOR).zip appalm-$(VERSION_MAJOR).$(VERSION_MINOR)
	rm -rf appalm-$(VERSION_MAJOR).$(VERSION_MINOR)

$(DSK2PDB): utils/dsk2pdb.c
	gcc -o $(DSK2PDB) $<

$(APPALM): $(APPALM_M68KEXECUTABLE) obj/appalm.bin.stamp
	$(BUILDPRC) $(APPALM) "Appalm ][" Aple2 $(APPALM_M68KEXECUTABLE) $(APPALM_BINS)

$(APPALM_M68KEXECUTABLE): $(APPALM_OBJS)
	$(GCC) $(CFLAGS) -o $(APPALM_M68KEXECUTABLE) $(APPALM_OBJS)

obj/appalm.bin.stamp: src/appalm.rcp
	$(PILRC) $^ obj
	touch $@

obj/appalm.o: src/appalm.c src/Apple2.h src/6502.h src/memory.h
	$(GCC) $(CFLAGS) -S $< -o $*.s
	$(GCC) $(CFLAGS) -c $< -o $*.o

obj/6502.o: src/6502.asm src/6502.h
	cpp -E $< > $*.s
	$(AS) $(AFLAGS) $*.s -o $*.o

obj/memio.o: src/memio.c src/memory.h src/iou.h
	$(GCC) $(CFLAGS) -S $< -o $*.s
	$(GCC) $(CFLAGS) -c $< -o $*.o

obj/video.o: src/video.c src/memory.h src/iou.h
	$(GCC) $(CFLAGS) -S $< -o $*.s
	$(GCC) $(CFLAGS) -c $< -o $*.o

obj/vidclr.o: src/vidclr.asm
	cpp -E $< > $*.s
	$(AS) $(AFLAGS) $*.s -o $*.o

obj/fonts7x8.o: src/fonts7x8.c
	$(GCC) $(CFLAGS) -c $< -o $*.o

obj/fonts4x6.o: src/fonts4x6.c
	$(GCC) $(CFLAGS) -c $< -o $*.o

$(A2MGR): $(A2MGR_M68KEXECUTABLE) obj/a2mgr.bin.stamp
	$(BUILDPRC) $(A2MGR) "A2 Manager" Disk2 $(A2MGR_M68KEXECUTABLE) $(A2MGR_BINS)

$(A2MGR_M68KEXECUTABLE): $(A2MGR_OBJS)
	$(GCC) $(CFLAGS) -o $(A2MGR_M68KEXECUTABLE) $(A2MGR_OBJS)

obj/a2mgr.bin.stamp: src/a2mgr.rcp
	$(PILRC) $^ obj
	touch $@

obj/a2mgr.o: src/a2mgr.c src/a2mgr_rsc.h
	$(GCC) $(CFLAGS) -c $< -o $*.o

