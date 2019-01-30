VWTEST_SRCS := \
		src/vwtest.c

VW_OBJS := $(VWTEST_SRCS:.c=.o)

OOBTEST_SRCS := \
		src/oobtest.c

OOB_OBJS := $(OOBTEST_SRCS:.c=.o)

LIBS :=
CFLAGS := -Iinclude -O2

TARGET_VWTEST := vwtest
TARGET_OOBTEST := oobtest
TARGET := $(TARGET_VWTEST) $(TARGET_OOBTEST)

all: $(TARGET)

%.o : %.c
	$(CC) -c $< -o $@ ${LDFLAGS} ${CFLAGS} ${LIBS}

$(TARGET_VWTEST): $(VW_OBJS)
	$(CC) $(VW_OBJS) -o $@ ${LIBS} ${LDFLAGS}

$(TARGET_OOBTEST): $(OOB_OBJS)
	$(CC) $(OOB_OBJS) -o $@ ${LIBS} ${LDFLAGS}

clean:
	rm -f *.o
