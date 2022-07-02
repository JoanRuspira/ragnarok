
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
1 - Clone and build the solution in this repo
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
      Extract and copy client data.rar of this repo into client side's /data folder
4 - Run Server's jRO and wait for it to finish
5 - Run Client's jRO
