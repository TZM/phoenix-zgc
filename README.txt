This is the Atrium Zeigeist Global Development site.
===================================================

Install the database and add the correct user permissions

here is how you do it from the command line

    $ mysql -u root -p
    Enter password: 
    Welcome to the MySQL monitor.  Commands end with ; or \g.
    Your MySQL connection id is 2837
    Server version: 5.5.15 Source distribution

    Copyright (c) 2000, 2010, Oracle and/or its affiliates. All rights reserved.

    Oracle is a registered trademark of Oracle Corporation and/or its
    affiliates. Other names may be trademarks of their respective
    owners.

    Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

    mysql>
    mysql> CREATE USER 'zgc'@'localhost' IDENTIFIED BY 'password';
    mysql> CREATE DATABASE zgc;
    mysql> GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, INDEX, ALTER ON zgc.* TO 'zgc'@'localhost' IDENTIFIED BY 'password';

Upload the latest database image
--------------------------------

    $ cd ~/sandboxes/
    $ git clone git://github.com/TZM/phoenix-db.git
    $ cd phoenix-db
    $ mysql -u root -p zgc < zgc.sql
    Password:

Create the web directory
------------------------

    $ cd /var/www
    $ git clone git://github.com/TZM/phoenix-zgc.git

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

Access to the atrium admin site
-------------------------------
    username:admin
    password:~I~|h(71927P_N?



