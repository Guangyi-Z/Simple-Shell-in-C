Simple Shell
=====


===Intro

Derived from the skeleton of the xv6 shell. 
The skeleton shell contains two main parts: parsing shell commands and implementing them. 

The parser recognizes only simple shell commands such as the following:

* ls > y
* cat < y | sort | uniq | wc > y1
* cat y1
* rm y1
* ls | sort | uniq | wc
* rm y

===Impl

getCmd
parseCmd
	parsePipe
	parseRedir
	parseExec
runCmd


