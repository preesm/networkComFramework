

TARGET=networkcom

INCDIR=includes
SRCDIR=src
OBJDIR=obj
SRCS=$(wildcard $(SRCDIR)/*.c)
INCS=$(wildcard $(INCDIR)/*.h)
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

LIBS=-lpthread

CC=gcc
CFLAGS=-O0 -g -I$(INCDIR)
#-Wextra


#PREESM_COM_PORT=$(shell awk 'BEGIN{srand();printf("%d", 6100*rand()+400)}')0
PREESM_COM_PORT=$(shell awk 'BEGIN{srand();printf("%d", 610*rand()+40)}')00

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR) $(INCS)
	$(CC) -c $(CFLAGS) -o $@ $< -DPREESM_COM_PORT=$(PREESM_COM_PORT)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LIBS)

clean:
	rm -rvf $(TARGET) $(OBJS) $(OBJDIR)

test: clean $(TARGET)
	#ulimit -n 8192
	./$(TARGET)
	@echo "\n - Success\n"

repeat:
	ulimit -n 8192
	set -e
	while true; do clear && make test && echo "------" || exit 1; sleep 1; done

valgrind: clean $(TARGET)
	valgrind ./$(TARGET)
	@echo "\n - Success\n"
