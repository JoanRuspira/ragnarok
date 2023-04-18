
FROM ubuntu:bionic
EXPOSE 6900
ENV LANG=en_GB.UTF-8
ENV RAGNAROK_DIR=/ragnarok

ENV MYSQL_ROOT_PW=root
ENV MYSQL_RAGNAROK_DB=rathena_re_db
ENV MYSQL_RAGNAROK_LOG_DB=rathena_re_log

ENV MYSQL_RAGNAROK_USER=ro_offline_user
ENV MYSQL_RAGNAROK_PW=ro_offline_pass

RUN apt-get update -y && apt-get upgrade -y
RUN apt-get install -y git make libmysqlclient-dev zlib1g-dev libpcre3-dev -y
RUN apt install build-essential -y
RUN apt-get install -y mysql-server mysql-client
RUN apt-get update -y
RUN apt-get install -y gcc-5
RUN sleep 10
RUN apt-get install -y g++-5
RUN sleep 10
RUN apt-get install -y python3
RUN apt-get install -y vim

COPY ./docker/do_things.sh /
RUN chmod +x /do_things.sh

RUN ["/do_things.sh"]
COPY . /ragnarok



# build image and spin up container
# docker build -t jro -f docker/Dockerfile .
# docker run -t -d --name=ragnarok -p 6900:6900 jro


# build server
# cd /ragnarok
# service mysql start
# chmod +x /ragnarok/configure
# /ragnarok/configure --enable-packetver=20200401
# make clean
# make server


# run server
# chmod +x /ragnarok/athena-start
# /ragnarok/athena-start



# view container ip address
# docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' ragnarok

# view container ports
# docker container ls --format "table {{.ID}}\t{{.Names}}\t{{.Ports}}" -a