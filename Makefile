all:
	gcc ftps.c -o ftps
	gcc ftpc.c -o ftpc
	gcc tcpd.c -o tcpd
	mkdir server_dir
	
run_troll:
	troll -C beta -S localhost -a 5558 -b 5556 -x 0 -g 0 -l 0 -m 0 -r -c 10000 5557

run_tcpd:
	tcpd

run_ftps:
	ftps 3333

run_ftpc:
	ftpc localhost 3333 $(file)

clean:
	rm -rf ftpc ftps tcpd
	rm -rf server_dir/*
	rmdir server_dir
	