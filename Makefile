PKGDIR  = .
L4DIR   ?= $(PKGDIR)/../..

TARGET := include server test

include $(L4DIR)/mk/subdir.mk

qemu:
	$(MAKE) && cd $(OBJ_BASE) && $(MAKE) qemu
