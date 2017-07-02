
#PROF    = -pg
PROF    = -lprofiler -std=gnu++11
OPT     = -O3

dozer: main.o dozer.o scoop.o pebble.o payload.o pebblepayload.o
	g++ ${OPT} -g -o dozer main.o dozer.o pebble.o scoop.o payload.o pebblepayload.o -I/usr/local/include -pthread -lboost_program_options -lboost_filesystem -L/usr/local/lib -L/usr/lib $(PROF)   -lstdc++

main.o: main.cpp assoc.hpp dozer.hpp
	g++ ${OPT} -g $(PROF) -c main.cpp

dozer.o: dozer.cpp assoc.hpp dozer.hpp scoop.hpp
	g++ ${OPT} -g $(PROF)  -I/usr/share/R/include -I/usr/lib/R/site-library/Rcpp/include -I/home/ivan/R/x86_64-pc-linux-gnu-library/2.14/RInside/include -c dozer.cpp

scoop.o: scoop.cpp pebble.hpp scoop.hpp
	g++ ${OPT} -g $(PROF) -c scoop.cpp

pebble.o: pebble.cpp pebble.hpp scoop.hpp
	g++ ${OPT} -g $(PROF) -c pebble.cpp

payload.o: payload.cpp payload.hpp
	g++ ${OPT} -g $(PROF) -c payload.cpp

pebblepayload.o: pebblepayload.cpp pebblepayload.hpp
	g++ ${OPT} -g $(PROF) -c pebblepayload.cpp




clear:
	rm *.o

sample:
	./dozer -s 15 -c .51 -t nom1  -i sample.bin

meaeod: meaeod.cpp
	g++ ${OPT} -g $(PROF) -I ~/repos/dozer/odeint/odeint -o meaeod meaeod.cpp -pthread -lboost_program_options -lboost_filesystem -lboost_system -lboost_date_time  -I/usr/share/R/include -L/usr/lib/R/lib -lR

testq: testq.cpp
	g++ ${OPT} -g $(PROF) -o testq testq.cpp -pthread -L/usr/local/lib -lboost_program_options -lboost_filesystem -lboost_system -lboost_date_time  -lopencv_core -lopencv_ml

testq.o: testq.cpp
	g++ ${OPT} -g $(PROF) -c testq.cpp -pthread -lboost_program_options -lboost_filesystem -lboost_system -lboost_date_time  -lopencv_core -lopencv_ml

