#!/bin/sh

BASE=0
COUNT_CC=$(sqlite3 serialgateway.db "SELECT count(*) FROM cmd_classes")
COUNT_CMD=$(sqlite3 serialgateway.db "SELECT count(*) FROM commands")

if [ "$COUNT_CC" -eq "$BASE" ] && [ "$COUNT_CMD" -eq "$BASE" ];
then
printf "populating command classes and commands table\n"
sqlite3 serialgateway.db <<EOF
.open serialgateway.db
.mode csv
.import cmd_class.csv cmd_classes
.import commands.csv commands
EOF

else
printf "SQLITE cmd_classes table check complete\n"
	
fi

