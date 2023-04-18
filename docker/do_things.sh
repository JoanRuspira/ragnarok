#!/bin/bash
git clone https://github.com/JoanRuspira/ragnarok.git $RAGNAROK_DIR
# git clone https://github.com/rathena/rathena.git $RAGNAROK_DIR

service mysql start

cleanup default mysql installation
echo "Cleaning up mysql installation..."
mysql -e "UPDATE mysql.user SET Password = PASSWORD('$MYSQL_ROOT_PW') WHERE User = 'root'"
mysql -e "DROP USER ''@'localhost'"
mysql -e "DROP USER ''@'$(hostname)'"
mysql -e "DROP DATABASE test"
mysql -e "FLUSH PRIVILEGES"
echo "Done!"
echo ""


# Create default ragnarok user and database
echo "Creating Database ${MYSQL_RAGNAROK_DB}..."
mysql -uroot -p${MYSQL_ROOT_PW} -e "CREATE DATABASE ${MYSQL_RAGNAROK_DB} /*\!40100 DEFAULT CHARACTER SET utf8 */;"
mysql -uroot -p${MYSQL_ROOT_PW} -e "CREATE DATABASE ${MYSQL_RAGNAROK_LOG_DB} /*\!40100 DEFAULT CHARACTER SET utf8 */;"
echo "Databases successfully created!"
echo ""


mysql -uroot -p${MYSQL_ROOT_PW} -e "CREATE USER ${MYSQL_RAGNAROK_USER}@localhost IDENTIFIED BY '${MYSQL_RAGNAROK_PW}';"
echo "User successfully created!"
echo ""
echo "Granting ALL privileges on ${MYSQL_RAGNAROK_DB} to ${MYSQL_RAGNAROK_USER}!"
mysql -uroot -p${MYSQL_ROOT_PW} -e "GRANT ALL PRIVILEGES ON ${MYSQL_RAGNAROK_DB}.* TO '${MYSQL_RAGNAROK_USER}'@'localhost';"
mysql -uroot -p${MYSQL_ROOT_PW} -e "GRANT ALL PRIVILEGES ON ${MYSQL_RAGNAROK_LOG_DB}.* TO '${MYSQL_RAGNAROK_USER}'@'localhost';"
mysql -uroot -p${MYSQL_ROOT_PW} -e "FLUSH PRIVILEGES;"
echo "Done!"


# import rathena sql files
mysql -uroot -p${MYSQL_ROOT_PW} ${MYSQL_RAGNAROK_DB} < ${RAGNAROK_DIR}/sql/rathena_re_db.sql
mysql -uroot -p${MYSQL_RAGNAROK_PW} ${MYSQL_RAGNAROK_LOG_DB} < ${RAGNAROK_DIR}/sql/rathena_re_log.sql
