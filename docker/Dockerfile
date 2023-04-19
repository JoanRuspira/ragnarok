
FROM ubuntu:bionic

EXPOSE 6900
EXPOSE 6121
EXPOSE 5121

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
RUN apt-get install -y g++-5
RUN apt-get install -y python3
RUN apt-get install -y vim

COPY ./initial_setup.sh /
RUN chmod +x /initial_setup.sh
RUN ["/initial_setup.sh"]

COPY ./entrypoint.sh /
ENTRYPOINT ["/entrypoint.sh"]



# build image and spin up container
# docker build -t jro -f Dockerfile .
# docker run -t -d --name=ragnarok -p 6900:6900 -p 6121:6121 -p 5121:5121 jro
