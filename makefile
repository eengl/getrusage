CPP = g++

.PHONEY:	clean

getrusage: getrusage.cpp
	$(CPP) $(CPPFLAGS) -o $@ getrusage.cpp

clean:
	-rm -f getrusage
