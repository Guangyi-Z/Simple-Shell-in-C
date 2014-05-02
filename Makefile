CC=gcc
MYCTAGS=-c -g  
MYLIBS=

# name your program
PROGRAM=a.out
SRCDIRS= .
SRCEXTS= .c .C
HDREXTS= .h .H

    SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))  
#    HEADERS = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(HDREXTS))))  
    OBJS    = $(addsuffix .o, $(basename $(SOURCES)))  
#    DEPS    = $(OBJS:.o=.d)  

.PHONY: all clean

all: $(PROGRAM)
$(PROGRAM):$(OBJS)
	$(CC) $(OBJS) -o $@ 

%.o:%.c
	$(CC) $(MYCTAGS) $< -o $@

include $(SOURCES:.c=.d)

%.d: %.c
	set -e; rm -f $@; \
		$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
			sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
				rm -f $@.$$$$

clean:
	rm $ $(OBJS) $(PROGRAM);
	rm *.d

