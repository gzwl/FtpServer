objs=assist.o echo.o event.o  work.o command.o configure.o ftp_string.o main.o

ftp : $(objs)
	  gcc $(objs) -o ftp
main.o : assist.h echo.h event.h configure.h common.h 
echo.o : echo.h common.h echo.c
assist.o : assist.h common.h echo.c assist.c
event.o : event.h work.h common.h 
configure.o : configure.h 
command.o : command.h echo.h common.h work.h configure.h 
work.o : command.h event.h common.h work.h echo.h ftp_string.h  
ftp_string.o : common.h ftp_string.h

.PHONY : clean
clean : 
		rm ftp $(objs)
