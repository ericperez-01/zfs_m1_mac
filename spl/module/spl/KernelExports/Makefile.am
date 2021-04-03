include $(top_srcdir)/config/Rules.am

AUTOMAKE_OPTIONS = subdir-objects


noinst_PROGRAMS = kextsymboltool

kextsymboltool_SOURCES = \
	kextsymboltool.c

kextsymboltool_LDFLAGS = -lstdc++

KernelExports: zfs.exports | kextsymboltool
	./kextsymboltool -arch arm64 -import allsymbols.txt -export zfs.exports -output KernelExports

clean:
	rm -f kextsymboltool.o kextsymboltool
	rm -f KernelExports

#allsymbols:
#	$(NM) -gj $(MACH_KERNEL) > allsymbols

all:kextsymboltool KernelExports #allsymbols KernelExports
