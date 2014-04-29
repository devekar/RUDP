SOURCES := timer.cpp functions.cpp
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)
EXECS := timer

all: $(EXECS)

clean: 
	${RM} $(OBJECTS) $(EXECS)

timer: timer.o functions.o
	g++ -o timer timer.o functions.o


%.o: %.cpp
	g++ -c $< -o $@

-include $(DEPS)
