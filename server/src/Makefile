PKGDIR  = ../..
L4DIR   ?= $(PKGDIR)/../..

REQUIRES_LIBS = libstdc++ libsupc++ libpthread libio-vbus
TARGET          = l4rdma
SRC_CC          = 	main.cc \
					wq_interface.cc \
					cq_interface.cc \
					server_interface.cc \
					device.cc \
					cmd.cc \
					mem.cc \
					driver.cc \
					event.cc \
					interrupt.cc \
					uar.cc \
					queue.cc \
					cq.cc \
					wq.cc

include $(L4DIR)/mk/prog.mk
