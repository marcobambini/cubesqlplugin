SDKDIR = ../CubeSQL_SDK
CRYPTDIR = ../CubeSQL_SDK/crypt
RBSDKDIR = ../Xojo_SDK
SRCDIR = ../sources
SDKGLUECODEDIR = $(RBSDKDIR)/Glue\ Code
PREFIXHEADER = $(RBSDKDIR)/Includes/LinuxHeader.h
INCLUDE = -I$(SRCDIR) -I$(SDKDIR)/ -I$(CRYPTDIR)/ -I$(RBSDKDIR)/Includes

CC = g++
LD = g++
CFLAGS = $(INCLUDE) -s -O2 -fPIC -DIGNOREQT -DLINUX=1 -DTARGET_LINUX -DDYNAMIC_SSL_LIBRAY=1 -DENABLE_SSL_ENCRYPTION=1
LDFLAGS = -shared -Wl,--no-undefined
LIBS = -lz -ldl
RM = /bin/rm -f

OBJS = CubeSQLPlugin.o cubesql.o pseudorandom.o aescrypt.o aeskey.o aestab.o base64.o sha1.o PluginMain.o

PROG = CubeSQLPlugin.so
all:	${PROG}

bit32:	CFLAGS += -m32
bit32:	LDFLAGS += -L/usr/lib32
bit32:	${PROG}

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

