PKGDIR  = ../..
L4DIR   ?= $(PKGDIR)/../..

REQUIRES_LIBS = libstdc++ libsupc++ libpthread libio-vbus
TARGET          = l4rdma
SRC_CC          = 	main.cc \
					wq.cc \
					cq.cc \
					server.cc \
					device.cc \
					cmd.cc \
					mem.cc \
					driver.cc \
					event.cc \
					interrupt.cc \
					uar.cc

include $(L4DIR)/mk/prog.mk
