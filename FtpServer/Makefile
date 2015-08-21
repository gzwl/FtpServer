objs=assist.o echo.o event.o  work.o command.o configure.o ftp_string.o transfer.o main.o

ftp : $(objs)
	  gcc $(objs) -DTEST -o ftp -lcrypt
main.o : assist.h echo.h event.h configure.h common.h 
echo.o : echo.h common.h  assist.h
assist.o : assist.h common.h 
event.o : event.h work.h common.h 
configure.o : configure.h 
command.o : command.h echo.h common.h work.h configure.h transfer.h
work.o : command.h event.h common.h work.h echo.h ftp_string.h  
ftp_string.o : common.h ftp_string.h
transfer.o : command.h common.h assist.h work.h transfer.h 



.PHONY : clean
clean : 
		rm ftp $(objs)
