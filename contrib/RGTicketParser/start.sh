#!/bin/bash
USER="root"
HOST="127.0.0.1"
PASSWORD="123"
DATABASE="dev_rg"

# Start main execution
rm runner.log
rm rg_custom_ticketstates.sql
java -jar "RGTicketParser.jar" > runner.log # Redirecting to log file
mysql -u $USER -p $PASSWORD -h $HOST $DATABASE < base.sql
mysql -u $USER -p $PASSWORD -h $HOST $DATABASE < rg_custom_ticketstates.sql 
