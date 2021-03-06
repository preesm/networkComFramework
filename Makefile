
NB_PE=64
TARGET=networkcom

INCDIR=includes
SRCDIR=src
OBJDIR=obj
SRCS=$(wildcard $(SRCDIR)/*.c)
INCS=$(wildcard $(INCDIR)/*.h)
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

LIBS=-lpthread

CC=gcc
CFLAGS=-O2  -I$(INCDIR) -D_PREESM_NBTHREADS_=$(NB_PE)  -Wno-unused-result
# -D_PREESM_TCP_DEBUG_
#-Wall -Wextra

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR) $(INCS)
	$(CC) -c $(CFLAGS) -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LIBS)

clean:
	rm -rvf $(TARGET) $(OBJS) $(OBJDIR)

test: clean $(TARGET)
	#ulimit -n 8192
	./$(TARGET)
	@echo "\n - Success\n"

repeat:
	#ulimit -n 8192
	set -e
	while true; do clear && make test && echo "------" || exit 1; sleep 0; done

valgrind: clean $(TARGET)
	valgrind ./$(TARGET)
	@echo "\n - Success\n"

testMP: clean $(TARGET)
	#ulimit -n $(NB_PE)0
	./runMP.sh $(TARGET) $(NB_PE)
	@echo "\n - Success\n"

repeatMP:
	set -e
	while true; do clear && make testMP && echo "------" || exit 1; sleep 0; done
