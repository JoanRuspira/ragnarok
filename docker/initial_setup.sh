#!/bin/bash

# clone server code
git clone https://github.com/JoanRuspira/ragnarok.git $RAGNAROK_DIR

# start mysql service
service mysql start

# cleanup default mysql installation
mysql -e "UPDATE mysql.user SET Password = PASSWORD('$MYSQL_ROOT_PW') WHERE User = 'root'"
mysql -e "DROP USER ''@'localhost'"
mysql -e "DROP USER ''@'$(hostname)'"
mysql -e "DROP DATABASE test"
mysql -e "FLUSH PRIVILEGES"

# Create databases
mysql -uroot -p${MYSQL_ROOT_PW} -e "CREATE DATABASE ${MYSQL_RAGNAROK_DB} /*\!40100 DEFAULT CHARACTER SET utf8 */;"
mysql -uroot -p${MYSQL_ROOT_PW} -e "CREATE DATABASE ${MYSQL_RAGNAROK_LOG_DB} /*\!40100 DEFAULT CHARACTER SET utf8 */;"

# Create server connection user
mysql -uroot -p${MYSQL_ROOT_PW} -e "CREATE USER ${MYSQL_RAGNAROK_USER}@localhost IDENTIFIED BY '${MYSQL_RAGNAROK_PW}';"
mysql -uroot -p${MYSQL_ROOT_PW} -e "GRANT ALL PRIVILEGES ON ${MYSQL_RAGNAROK_DB}.* TO '${MYSQL_RAGNAROK_USER}'@'localhost';"
mysql -uroot -p${MYSQL_ROOT_PW} -e "GRANT ALL PRIVILEGES ON ${MYSQL_RAGNAROK_LOG_DB}.* TO '${MYSQL_RAGNAROK_USER}'@'localhost';"
mysql -uroot -p${MYSQL_ROOT_PW} -e "FLUSH PRIVILEGES;"

# import sql files containig db structure and admin/interaction accounts
mysql -uroot -p${MYSQL_ROOT_PW} ${MYSQL_RAGNAROK_DB} < ${RAGNAROK_DIR}/sql/rathena_re_db.sql
mysql -uroot -p${MYSQL_RAGNAROK_PW} ${MYSQL_RAGNAROK_LOG_DB} < ${RAGNAROK_DIR}/sql/rathena_re_log.sql
