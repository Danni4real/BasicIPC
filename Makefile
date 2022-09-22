SRCPATH  = .
EXE      = test
CFLAGS   = -g -fPIC
LDFLAGS  = -lpthread

GCC   = gcc
GPLUS = g++ 
RM    = rm -rf

SRCS = $(wildcard $(SRCPATH)/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(SRCS))

all: $(EXE) 
%.o:%.cpp
	$(GPLUS)  -c $< -o $@ $(CFLAGS)
%.s:%.cpp
	$(GPLUS)  -S $< -o $@ $(CFLAGS)
$(EXE):$(OBJS)
	$(GPLUS)     $^ -o $@ $(LDFLAGS)
	$(RM) $(SRCPATH)/*.o
	
.PHONY:clean
clean:
	$(RM) $(SRCPATH)/*.o $(EXE)
	
