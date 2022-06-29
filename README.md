

## 1. Setup
1 - Clone and build the solution in this repo (https://github.com/JoanRuspira/ragnarok)
2 - Download OpenServer at https://mega.nz/file/JUc2VTjK#01MwBbPdj657aUV3BskTHKSaDQgt72pVd7k-hBwVZy0
      Unzip and run OpenServer
      Start server by Clicking "Run Server" in the bottom right toolbar (Red flag)
      Click on the now green flag -> "Advanced" -> "PHPMyAdmin"
      Login (default: root/root)
      Create a new row in the "login" table of the "rathena/re/rathena_re_db" database:
        userid: inter_user
        user_pass: inter_pass
        sex: S
        email: athena@athena.com
        group_id: 0
3 - Download Client side at https://mega.nz/file/lJ1RnZoI#10xZx-DQeMQTP3kM4mFjuBrjtN4XmU4C0czOYCn2JTY
      Download Client data at https://mega.nz/file/kdMVBDyC#icOT6o-Jl-IIEpi8cqsyMRd-0lXvOSfbeSlUsx4Y4is
      Extract and copy client data into client side's /data folder
4 - Run Server's jRO and wait for it to finish
5 - Run Client's jRO


### Hardware
Hardware Type | Minimum | Recommended
CPU | 1 Core | 2 Cores
RAM | 1 GB | 2 GB
Disk Space | 300 MB | 500 MB

### Operating System & Preferred Compiler
Operating System | Compiler
Windows | [MS Visual Studio 2017](https://www.visualstudio.com/downloads/)

### Required Applications
Application | Name
Database | [MySQL 5 or newer](https://www.mysql.com/downloads/) 
Git | [Windows](https://gitforwindows.org/) 