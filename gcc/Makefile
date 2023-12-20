SDKDIR = ../CubeSQL_SDK
CRYPTDIR = ../CubeSQL_SDK/crypt
RBSDKDIR = ../Xojo_SDK
SRCDIR = ../sources
SDKGLUECODEDIR = $(RBSDKDIR)/GlueCode
PREFIXHEADER = $(RBSDKDIR)/Includes/LinuxHeader.h
INCLUDE = -I$(SRCDIR) -I$(SDKDIR)/ -I$(CRYPTDIR)/ -I$(RBSDKDIR)/Includes

LINUX_ARM32_LIB = ../libs/linux-arm
LINUX_ARM64_LIB = ../libs/linux-arm64
LINUX_32BIT_LIB = ../libs/linux
LINUX_64BIT_LIB = ../libs/linux-64

LINUX_LIBPATH = $(LINUX_ARM32_LIB)

CC = g++
LD = g++
CFLAGS = $(INCLUDE) -s -O2 -std=c++11 -Wno-narrowing -fPIC -DIGNOREQT -DLINUX=1 -DTARGET_LINUX -DCUBESQL_ENABLE_SSL_ENCRYPTION=1
LDFLAGS = -shared -Wl,--no-undefined -L$(LINUX_LIBPATH)
LIBS = -lz -ldl -lpthread -ltls
RM = /bin/rm -f

OBJS = CubeSQLPlugin.o cubesql.o pseudorandom.o aescrypt.o aeskey.o aestab.o base64.o sha1.o PluginMain.o

PROG = CubeSQLPlugin.so
all:	${PROG}


${PROG}:	${OBJS}
	${LD} ${CFLAGS} ${LDFLAGS} ${OBJS} -o ${PROG} ${LIBS}

CubeSQLPlugin.o:	$(SRCDIR)/CubeSQLPlugin.cpp
	${CC} $(CFLAGS) -include $(PREFIXHEADER) -c $< -o $@

PluginMain.o: $(SDKGLUECODEDIR)/PluginMain.cpp
	$(CC) $(CFLAGS) -include $(PREFIXHEADER) $(SDKGLUECODEDIR)/PluginMain.cpp -c

cubesql.o:	$(SDKDIR)/cubesql.c
	${CC} $(CFLAGS) -c $< -o $@

%.o:	$(CRYPTDIR)/%.c
	${CC} $(CFLAGS) -c $< -o $@

clean:	
	${RM} ${PROG} ${OBJS}

