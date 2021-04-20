CFLAGS=-Wall -Werror -g
SDIR=src
ODIR=build
EXECUTABLE=$(ODIR)/kernalemu

_OBJS = console.o cbmdos.o screen.o memory.o time.o ieee488.o channelio.o io.o keyboard.o printer.o vector.o c128.o main.o dispatch.o fake6502.o

_HEADERS = c128.h cbmdos.h channelio.h console.h dispatch.h error.h fake6502.h glue.h ieee488.h io.h keyboard.h memory.h printer.h readdir.h screen.h stat.h time.h vector.h

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
HEADERS = $(patsubst %,$(SDIR)/%,$(_HEADERS))

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS) $(HEADERS)
	$(CC) -o $(EXECUTABLE) $(OBJS) $(LDFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	@mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(ODIR)
