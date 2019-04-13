
all:
	g++ config.cpp file-cte.cpp file-log.cpp main.cpp -o ecu2map
	
clean:
	rm -rf *.o ecu2map