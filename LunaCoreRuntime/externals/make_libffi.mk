INSTALL_PATH	:=	$(CURDIR)/../ffi

OUTPUT			:= $(INSTALL_PATH)/lib/libffi.a

$(OUTPUT): configure
	@./configure --prefix $(CURDIR)/../ffi --host=arm-none-eabi --enable-static --disable-docs \
		CFLAGS="-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -marm"
	@$(MAKE) --no-print-directory
	@$(MAKE) --no-print-directory install
	@rm arm-none-eabi/include/ffi.h arm-none-eabi/include/ffitarget.h arm-none-eabi/config.status arm-none-eabi/local.exp

configure:
	@./autogen.sh

clean:
	@echo clean...
	@$(MAKE) distclean
	@rm -f $(OUTPUT)