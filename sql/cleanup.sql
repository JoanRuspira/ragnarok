



# cleanup default mysql installation
echo "Cleaning up mysql installation..."
mysql -e "UPDATE mysql.user SET Password = PASSWORD('$MYSQL_ROOT_PW') WHERE User = 'root'"
mysql -e "DROP USER ''@'localhost'"
mysql -e "DROP USER ''@'$(hostname)'"
mysql -e "DROP DATABASE test"
mysql -e "FLUSH PRIVILEGES"
echo "Done!"
echo ""