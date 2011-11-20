This is the Zeigeist Global Development site.
=============================================


You will need to clone the database file from

$ git clone git@git.x11.us:zeitgeist-db

then install this database and add the correct user permissions

here is how you do it from the command line, sorry Pete F.

$ mysql -u root -p
Enter password: 
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 108
Server version: 5.1.41-3ubuntu12.10 (Ubuntu)

mysql> CREATE USER 'zgc'@'localhost' IDENTIFIED BY 'password';
mysql> CREATE DATABASE zgc;
mysql> GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, INDEX, ALTER ON zgc.* TO 'zgc'@'localhost' IDENTIFIED BY 'password';

Create the web directory
------------------------

$ cd /var/www
$ git clone git@git.x11.us:phoenix-zgc

=======================================
# PLEASE DO NOT CHANGE THE .gitignore #
=======================================

then you will need to:

$ cp sites/default/default.settings.php sites/default/settings.php

and change the password to whatever you when you created the database user. but as it is on your local machine it does not really matter.

$db_url = 'mysql://zgc:password@localhost/zgc';

Configure Apache/Nginx and /etc/hosts
-------------------------------------

You will need to setup a zgc.local in /etc/hosts so that it resolves to 127.0.0.1

You will then need to update your config file for the web server you're using.

Hopefully, once we sort the VM image correctly, you will just need to load this up.
