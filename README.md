## Windows Setup
### Prerequisites
[MS Visual Studio](https://www.visualstudio.com/downloads/)
[MySQL5](install exe in Mega account, set root user & password as 'root', set mysql port to 3306)
[Windows](https://gitforwindows.org/)
Visual studio C++ 2012 Redistributable (install exes in this repo)
Visual studio C++ 2015 Redistributable (install exes in this repo)

### Process
1 - Clone the solution in this repo and build it with Visual Studio.

2 - Start the MySql service (from running services.msc)

3 - Create user 'ro_offline_user' with password 'ro_offline_pass' with dbAdmin privileges 

4 - Create 'rathena_re_db' and 'rathena_log_db' with 'utf8' charset

5 - run scripts in /sql against their respective database to create db structure

6 - Download Client from Mega account
      Extract and copy data.rar of this repo into client's /data folder
      Extract and copy System.rar of this repo into client's /System folder

7 - Set address field in /data/clientinfo.xml (client) to 127.0.0.1 and the port to 6900.
    The server needs TCP ports 5121, 6121 and 6900 open.

8 - Run Server's jRO.bat and wait for it to finish

9 - Run Client's jRO.exe


## Docker Setup (assumes Docker is installed)
### Prerequisites
[Docker](https://www.docker.com/products/docker-desktop/)

### Process
1 - Clone the solution in this repo

2 - Download Client from Mega account
      Extract and copy data.rar of this repo into client's /data folder
      Extract and copy System.rar of this repo into client's /System folder

3 - Set address field in /data/clientinfo.xml (client) to 127.0.0.1 and the port to 6900.
    The server needs TCP ports 5121, 6121 and 6900 open.

4 - Cd to /docker and build the Docker image with the following command
      docker build -t jro -f Dockerfile .

5 - Spin up a container from the docker image (check container logs and wait for entrypoint to finish)
      docker run -t -d --name=ragnarok -p 6900:6900 -p 6121:6121 -p 5121:5121 jro
      ("docker container stop container_id" to stop the container)
      ("docker start container_id" to restart the container)
      ("docker logs container_id" to check the container logs)

6 - Run Client's jRO.exe

