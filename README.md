
### Hardware
Hardware Type | Minimum | Recommended
CPU           | 1 Core  | 2 Cores
RAM           | 1 GB    | 2 GB
Disk Space    | 300 MB  | 500 MB

### Required Applications
Compiler | [MS Visual Studio 2017](https://www.visualstudio.com/downloads/)
Database | [MySQL 5 or newer](https://www.mysql.com/downloads/) 
Git      | [Windows](https://gitforwindows.org/) 

## Setup
1 - Clone the solution in this repo and build it with Visual Studio 2017 (or download a built server from Mega account).
2 - Download OpenServer from Mega account
      Unzip and run OpenServer
      Start server by Clicking "Run Server" in the bottom right toolbar (Red flag)
      Click on the now green flag -> "Advanced" -> "PHPMyAdmin"
      Login (default: root/root)
      Make sure a row in like this exists in the "login" table of the "rathena/re/rathena_re_db" database:
        userid: inter_user
        user_pass: inter_pass
        sex: S
        email: athena@athena.com
        group_id: 0
3 - Download Client side from Mega account
      Extract and copy data.rar of this repo into client side's /data folder
      Extract and copy System.rar of this repo into client side's /System folder
4 - Make sure Visual studio C++ 2012 Redistributable is installed on server (install exes in this repo).
	Make sure Visual studio C++ 2015 Redistributable is installed on server (install exes in this repo).
5 - If running client and server in separate machines in different networks:
		Access server and open TCP ports 5121, 6121 and 6900 in Windows firewall. (Create inbound rule to open them)
		Set address field in /data/clientinfo.xml (client side) to the static ip of the server.
	If running client and server in the same machine:
		Set address field in /data/clientinfo.xml (client side) to 127.0.0.1.
	If running client and server in the separate machines in the same network:
		Set address field in /data/clientinfo.xml (client side) to local ip of the server.	
6 - Run Server's jRO and wait for it to finish
7 - Run Client's jRO
